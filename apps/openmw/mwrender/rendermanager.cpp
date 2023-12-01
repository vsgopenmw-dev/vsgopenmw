#include "rendermanager.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>

#include <vsg/app/CommandGraph.h>
#include <vsg/app/CompileManager.h>
#include <vsg/app/RenderGraph.h>
#include <vsg/io/write.h>
#include <vsg/vk/Context.h>
#include <vsg/threading/OperationThreads.h>
#include <vsg/core/Objects.h>
#include <vsg/utils/ComputeBounds.h>

#include <components/animation/transform.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/mwanimation/color.hpp>
#include <components/mwanimation/object.hpp>
#include <components/pipeline/sets.hpp>
#include <components/pipeline/builder.hpp>
#include <components/render/download.hpp>
#include <components/render/engine.hpp>
#include <components/render/screen.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/settings.hpp>
#include <components/terrain/paging.hpp>
#include <components/view/barriers.hpp>
#include <components/view/collectlights.hpp>
#include <components/view/scene.hpp>
#include <components/view/defaultstate.hpp>
#include <components/view/descriptors.hpp>
#include <components/view/lightgrid.hpp>
#include <components/view/scene.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/vsgutil/updatethreads.hpp>
#include <components/vsgutil/compileop.hpp>
#include <components/vsgutil/bounds.hpp>
#include <components/vsgutil/computebin.hpp>
#include <components/vsgutil/nullbin.hpp>
#include <components/vsgutil/readnode.hpp>
#include <components/vsgutil/sharedview.hpp>
#include <components/vsgutil/suspendrenderpass.hpp>
#include <components/vsgutil/compilecontext.hpp>
#include <components/vsgutil/projection.hpp>
#include <components/vsgutil/setbin.hpp>
// #include <components/detournavigator/navigator.hpp>

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"

#include "renderpass.hpp"
#include "bin.hpp"
#include "animcontext.hpp"
#include "camera.hpp"
#include "effects.hpp"
#include "env.hpp"
#include "fogmanager.hpp"
#include "mask.hpp"
#include "player.hpp"
#include "projectiles.hpp"
#include "reflect.hpp"
#include "scene.hpp"
#include "screenshotinterface.hpp"
#include "shadow.hpp"
#include "simplewater.hpp"
#include "terrainstorage.hpp"
#include "water.hpp"
#include "weather.hpp"
#include "map.hpp"
#include "worldmap.hpp"
#include "preview.hpp"
#include "intersect.hpp"

namespace MWRender
{
    namespace
    {
        vsg::ref_ptr<vsg::Group> createCastShadowState()
        {
            vsg::Descriptors descriptors = View::createLightingDescriptors(vsg::vec4Array::create(2), false);
            descriptors.emplace_back(View::dummyEnvMap());
            descriptors.emplace_back(View::dummyShadowMap());
            descriptors.emplace_back(View::dummyShadowSampler());

            View::Scene scene;
            descriptors.push_back(scene.descriptor());

            auto viewDescriptorSet = View::createViewDescriptorSet(descriptors);
            return View::createDefaultState(viewDescriptorSet);
        }
    }

    RenderManager::RenderManager(Render::Engine& engine, vsg::ref_ptr<vsg::Node> gui,
        Resource::ResourceSystem* resourceSystem /*, DetourNavigator::Navigator& navigator*/)
        : mEngine(engine)
        , mResourceSystem(resourceSystem)
        //, mNavigator(navigator)
        , mFog(new FogManager)

        , mMinimumAmbientLuminance(
              std::clamp(Settings::Manager::getFloat("minimum interior brightness", "Shaders"), 0.f, 1.f))
        , mNearClip(Settings::Manager::getFloat("near clip", "Camera"))
        , mViewDistance(Settings::Manager::getFloat("viewing distance", "Camera"))
        , mFieldOfView(std::clamp(Settings::Manager::getFloat("field of view", "Camera"), 1.f, 179.f))
    {
        vsg::ref_ptr<vsg::Context> context = vsg::Context::create(mEngine.getOrCreateDevice());
        mEngine.setRenderPass(createMainRenderPass(context->device, mEngine.framebufferSamples(), mEngine.getPresentLayout()));

        const int numLoadingThreads = 1;
        //if (compileManager->numCompileTraversals > 1)
        //    numLoadingThreads = std::min(nunLoadingThreads, 1);
        mOperationThreads = vsg::OperationThreads::create(numLoadingThreads);

        mRenderModes.resize(Render_Count);
        mCamera.reset(new Camera);

        mShadow = std::make_unique<Shadow>(*context, mEngine.getEnabledFeatures().depthClamp);
        mReflect = std::make_unique<Reflect>(mEngine);
        mScreenshotInterface = std::make_unique<ScreenshotInterface>();

        mSceneData = std::make_unique<View::Scene>();
        auto collectLights = vsg::ref_ptr{ new View::CollectLights(256) };
        vsg::Descriptors descriptors = View::createLightingDescriptors(collectLights->data(), true);
        descriptors.emplace_back(readEnv(resourceSystem->textureOptions));
        if (auto shadowMap = mShadow->shadowMap())
        {
            descriptors.emplace_back(shadowMap);
            descriptors.emplace_back(mShadow->shadow->shadowSampler());
        }
        else
        {
            descriptors.emplace_back(View::dummyShadowMap());
            descriptors.emplace_back(View::dummyShadowSampler());
        }
        descriptors.emplace_back(mReflect->reflectColor());
        descriptors.emplace_back(mReflect->reflectDepth());
        descriptors.emplace_back(mSceneData->descriptor());

        auto viewDescriptorSet = View::createViewDescriptorSet(descriptors);
        mSceneRoot = View::createDefaultState(viewDescriptorSet);

        mPlayerGroup = vsg::Switch::create();

        mProjectionMatrix = vsg::Perspective::create();
        mViewMatrix = vsg::LookAt::create();
        mSceneCamera
            = vsg::Camera::create(mProjectionMatrix, mViewMatrix, vsg::ViewportState::create(mEngine.extent2D()));

        auto resourceHints = vsg::ResourceHints::create();
        uint32_t setsPerPool = 64;
        resourceHints->numDescriptorSets = setsPerPool;
        resourceHints->descriptorPoolSizes = {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1 },
            //, builder->graphics->descriptorSetLayout {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2*setsPerPool },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4*setsPerPool },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7*setsPerPool }
            // }
        };
        vsg::ResourceRequirements resourceRequirements(resourceHints);
        mCompileContext = mEngine.createCompileContext();

        mScene = std::make_unique<Scene>(Settings::Manager::getInt("animation threads", "Cells"));
        mScene->context = animContext(resourceSystem, mCompileContext);
        mScene->noParticlesContext = mScene->context;
        mScene->noParticlesContext.mask.particle = vsg::MASK_OFF;

        mEffects = std::make_unique<Effects>(mScene->context);
        mProjectiles = std::make_unique<Projectiles>(mScene->context);
        mSky = std::make_unique<Sky>(*resourceSystem, mCompileContext->clone(Mask_Sky), mEngine.getEnabledFeatures().occlusionQueryPrecise);
        mWater = std::make_unique<Water>(resourceSystem);
        mSimpleWater = std::make_unique<SimpleWater>(Constants::CellSizeInUnits, *mResourceSystem->builder, mResourceSystem->textureOptions);

        mTerrainStorage = std::make_unique<TerrainStorage>(mResourceSystem);
        mTerrain = std::make_unique<Terrain::Paging>(mCompileContext->clone(Mask_Terrain), mResourceSystem->textureOptions, mResourceSystem->shaderOptions, mResourceSystem->builder->createSampler());
        mTerrain->lodFactor = std::max(0.00001f, Settings::Manager::getFloat("lod factor", "Terrain"));
        mTerrain->vertexLodMod = Settings::Manager::getInt("vertex lod mod", "Terrain");

        mTerrainView = mTerrain->createView();
        mPreview = std::make_unique<Preview>(*context, mEngine.framebufferSamples(), mScene->context, mResourceSystem);
        mWorldMap = std::make_unique<WorldMap>(mCompileContext, mResourceSystem->shaderOptions, mOperationThreads);
        mMap = std::make_unique<Map>(*context, mEngine.framebufferSamples(), mCompileContext, createMapScene(), mTerrain.get(), mViewDistance,
            Settings::Manager::getInt("local map resolution", "Map"), mResourceSystem->shaderOptions);
        auto guiRenderTextures = vsg::Group::create();
        guiRenderTextures->children.emplace_back(mPreview->node());
        guiRenderTextures->children.emplace_back(mMap->getNode());
        guiRenderTextures->children.emplace_back(mMap->getFogNode());
        guiRenderTextures->children.emplace_back(mWorldMap->node());

        mLightGrid
            = std::make_unique<View::LightGrid>(*resourceSystem->shaderOptions, viewDescriptorSet);
        auto computeGroup = vsg::Group::create();
        computeGroup->children = { View::preComputeBarrier(), mLightGrid->node(), mSky->resetNode() };
        auto computeBinContents = vsgUtil::createBinSetter(Bin_Compute, computeGroup);
        auto viewRoot = vsg::Group::create();
        viewRoot->children = { computeBinContents, mSceneRoot };
        mView = vsgUtil::createSharedView(mSceneCamera, viewRoot);
        mView->viewDependentState = collectLights;
        mView->mask = ~Mask_GUI;
        mView->bins.push_back(vsg::Bin::create(Bin_DepthSorted, vsg::Bin::DESCENDING));

        mScreenshotSwitch = vsg::Switch::create();

        mRenderGraph = mEngine.createRenderGraph();
        mRenderGraph->children = { mView };
        mSuspend = vsg::ref_ptr{ new vsgUtil::SuspendRenderPass(mRenderGraph) };

        mGuiRenderGraph = vsg::RenderGraph::create();
        mGuiRenderGraph->framebuffer = mRenderGraph->framebuffer;
        mGuiRenderGraph->window = mRenderGraph->window;
        mGuiRenderGraph->renderPass = createGuiRenderPass(context->device, mEngine.framebufferSamples(), mEngine.getPresentLayout());
        mGuiRenderGraph->renderArea = mRenderGraph->renderArea;
        mGuiRenderGraph->children = { gui };

        vsg::CommandGraphs threads;

        auto guiCommandGraph = mEngine.createCommandGraph(Pipeline::COMPUTE_SET);
        guiCommandGraph->children = { guiRenderTextures, mGuiRenderGraph };
        guiCommandGraph->submitOrder = 1;
        threads.push_back(guiCommandGraph);

        auto computeBin = vsgUtil::ComputeBin::create(); // VUID-vkCmdDispatch-renderpass
        computeBin->binNumber = Bin_Compute;
        computeBin->commandGraph = mEngine.createCommandGraph(Pipeline::COMPUTE_SET);
        computeBin->commandGraph->submitOrder = -1;
        mView->bins.push_back(computeBin);

        if (mRenderGraph->window)
            mCompileContext->compileManager->add(*mRenderGraph->window, mView, resourceRequirements);
        else
            mCompileContext->compileManager->add(*mRenderGraph->framebuffer, mView, resourceRequirements);

        mCommandGraph = mEngine.createCommandGraph(Pipeline::COMPUTE_SET);
        mCommandGraph->children = { View::postComputeBarrier(), mRenderGraph, mScreenshotSwitch };
        threads.push_back(mCommandGraph);

        for (int i = 0; mShadow->shadow && i < mShadow->shadow->numCascades; ++i)
        {
            auto rg = mShadow->shadow->renderGraph(i);
            auto castShadowGraph = mEngine.createCommandGraph(Pipeline::COMPUTE_SET);
            castShadowGraph->submitOrder = -1;
            auto view = mShadow->shadow->cascadeView(i);
            view->mask = mShadow->viewMask();
            view->bins = {
                vsg::Bin::create(Bin_DepthSorted, vsg::Bin::NO_SORT),
                vsgUtil::NullBin::create(Bin_Compute)
            };
            auto root = createCastShadowState();
            root->children = { view };
            rg->children = { root };
            castShadowGraph->children = { rg };
            threads.push_back(castShadowGraph);
            mRecordShadow.push_back(castShadowGraph->recordTraversal);
            if (i == 0)
                mCompileContext->compileManager->add(*rg->framebuffer, view);
        }

        mRecord = mCommandGraph->recordTraversal;

        mEngine.setup(threads, getenv("VSGOPENMW_NO_VIEWER_THREADING") == 0);

        updateProjectionMatrix();

        /*
         * Asynchronously compiles pipeline-heavy objects.
         */
        //updateSceneChildren();
        auto objectsToCompile = vsg::Objects::create();
        objectsToCompile->children = {
            mWater->node(),
            mSky->node()
        };
        mCompileOp = new vsgUtil::CompileOp(objectsToCompile, mCompileContext->clone(Mask_Water|Mask_Sky));
        mOperationThreads->add(mCompileOp);
        createIntersectorScene();
        setupScreenshotInterface();
    }

    RenderManager::~RenderManager()
    {
        if (mCompileOp)
            mCompileOp->wait();
    }

    void RenderManager::setStore(const MWWorld::ESMStore& store)
    {
        mTerrainStorage->setStore(&store);
        mTerrain->setStorage(mTerrainStorage.get());
    }

    RenderingInterface& RenderManager::getObjects()
    {
        return *mScene;
    }

    vsg::ref_ptr<vsg::Node> RenderManager::createMapScene()
    {
        auto group = vsg::Group::create();
        group->children = { mScene->getObjects(), mSimpleWater->node() };
        return group;
    }

    vsg::Group::Children RenderManager::createCastShadowScene(bool exterior)
    {
        vsg::Group::Children children;
        auto cat = "Shadows";
        if (!exterior && !Settings::Manager::getBool("enable indoor shadows", cat))
            return children;

        if (Settings::Manager::getBool("terrain shadows", cat))
            children.push_back(mTerrainView->node());
        if (exterior && Settings::Manager::getBool("object shadows", cat))
            children.push_back(mScene->getObjects());
        if (Settings::Manager::getBool("actor shadows", cat))
            children.push_back(mScene->getActors());
        if (Settings::Manager::getBool("player shadows", cat))
            children.push_back(mPlayerGroup);
        return children;
    }

    void RenderManager::ensureCompiled()
    {
        if (mCompileOp)
        {
            mCompileOp->ensure();
            mCompileOp = {};
        }
    }

    void RenderManager::updateSceneChildren()
    {
        ensureCompiled();
        mSceneRoot->children.clear();
        if (mRenderModes[Render_Sky])
            mSceneRoot->addChild(mSky->node());
        if (!mRenderModes[Render_Scene])
            return;
        if (mRenderModes[Render_Terrain])
            mSceneRoot->addChild(mTerrainView->node());
        mSceneRoot->addChild(mPlayerGroup);
        mSceneRoot->addChild(mScene->getObjects());
        mSceneRoot->addChild(mScene->getActors());
        if (mRenderModes[Render_Water])
        {
            mSuspend->children = { mReflect->node() };
            mSceneRoot->addChild(mSuspend);
            mSceneRoot->addChild(mWater->node());
        }
        mSceneRoot->addChild(mProjectiles->node());
        mSceneRoot->addChild(mEffects->node());
    }

    void RenderManager::createIntersectorScene()
    {
        mIntersectorScene = vsg::Group::create();
        mIntersectorScene->children = { mTerrainView->node(), mPlayerGroup, mScene->getObjects(), mScene->getActors() };
    }

    void RenderManager::setViewMode(ViewMode mode, bool enable)
    {
        vsg::Mask mask;
        if (mode == ViewMode::Scene)
            mask = ~Mask_GUI;

        auto& traversalMask = mRecord->traversalMask;
        if (enable)
            traversalMask |= mask;
        else
            traversalMask &= ~mask;
        for (auto& traversal : mRecordShadow)
            traversal->traversalMask = enable ? mask : vsg::MASK_OFF;
    }

    bool RenderManager::toggleRenderMode(RenderMode mode)
    {
        bool enabled = mRenderModes[mode].toggled = !mRenderModes[mode].toggled;
        updateSceneChildren();
        return enabled;
    }

    void RenderManager::setRenderModeActive(RenderMode mode, bool active)
    {
        if (mRenderModes[mode].active != active || mSceneRoot->children.empty())
        {
            mRenderModes[mode].active = active;
            updateSceneChildren();
        }

        if (!active && mode == Render_Terrain)
            mTerrain->clearView(*mTerrainView);
    }

    Resource::ResourceSystem* RenderManager::getResourceSystem()
    {
        return mResourceSystem;
    }

    const MWAnim::Context& RenderManager::getAnimContext()
    {
        return mScene->context;
    }

    void RenderManager::setNightEyeFactor(float factor)
    {
        mNightEyeFactor = factor;
    }

    void RenderManager::setAmbientColour(const vsg::vec4& colour)
    {
        mAmbientColor = colour;
    }

    void RenderManager::skySetDate(int day, int month)
    {
        mSky->setDate(day, month);
    }

    int RenderManager::skyGetMasserPhase() const
    {
        return mSky->getMasserPhase();
    }

    int RenderManager::skyGetSecundaPhase() const
    {
        return mSky->getSecundaPhase();
    }

    void RenderManager::skySetMoonColour(bool red)
    {
        mSky->setMoonColour(red);
    }

    void RenderManager::configureAmbient(const MWWorld::Cell& cell)
    {
        auto ambient = vsg::vec4(MWAnim::rgbColor(cell.getMood().mAmbiantColor), 1.f);
        //if (!cell.isExterior() && !cell.isQuasiExterior())
            //MWAnim::brighten(ambient, mMinimumAmbientLuminance);
        setAmbientColour(ambient);

        auto diffuse = vsg::vec4(MWAnim::rgbColor(cell.getMood().mDirectionalColor), 1.f);
        mLightDiffuse = diffuse;
        mLightSpecular = diffuse;
        mLightPosition = vsg::vec3(-0.15f, 0.15f, 1.f);
    }

    void RenderManager::setSunColour(const osg::Vec4f& diffuse, const osg::Vec4f& specular)
    {
        mLightDiffuse = toVsg(diffuse);
        mLightSpecular = toVsg(specular);
    }

    void RenderManager::setSunDirection(const osg::Vec3f& direction)
    {
        osg::Vec3 position = direction * -1;
        mLightPosition = toVsg(position);
        mSky->setSunDirection(position);
    }

    void RenderManager::addCell(const MWWorld::CellStore* store)
    {
        /*
        mPathgrid->addCell(store);
        */
    }
    void RenderManager::removeCell(const MWWorld::CellStore* store)
    {
        /*
        mPathgrid->removeCell(store);
        mActorsPaths->removeCell(store);
        */
        mScene->removeCell(store);
    }

    void RenderManager::updateTerrain(const vsg::dvec3& pos)
    {
        if (auto compile = mTerrain->updateView(*mTerrainView, pos, mViewDistance))
            compile->compile();
    }

    void RenderManager::changeCellGrid(const osg::Vec3f& pos)
    {
        updateTerrain(vsg::dvec3(toVsg(pos)));
    }

    void RenderManager::pruneCache()
    {
        getOperationThreads()->add(vsg::ref_ptr{ new vsgUtil::OperationFunc([this]() {
            mResourceSystem->updateCache(0);
            mTerrain->pruneCache();
            mScene->context.prune();
            //mPreview->mContext->prune();
        }) });
    }

    Sky* RenderManager::getSky()
    {
        ensureCompiled();
        return mSky.get();
    }

    void RenderManager::setSkyEnabled(bool enabled)
    {
        setRenderModeActive(Render_Sky, enabled);

        auto &shadow = mShadow->shadow;
        for (int i = 0; shadow && i < shadow->numCascades; ++i)
            shadow->cascadeView(i)->children = createCastShadowScene(enabled);
    }

    bool RenderManager::toggleBorders()
    {
        /*
        bool borders = !mTerrain->getBordersVisible();
        mTerrain->setBordersVisible(borders);
        return borders;
        */
        return false;
    }

    void RenderManager::updateSceneData()
    {
        bool isUnderwater
            = mRenderModes[Render_Water].active && mWater->isUnderwater(vsg::vec3(toVsg(mCamera->getPosition())));
        auto fogColor = mFog->getFogColor(isUnderwater);
        mRenderGraph->setClearValues(VkClearColorValue{ { fogColor.x, fogColor.y, fogColor.z, 1 } });

        mSceneData->setFogRange(mFog->getFogStart(isUnderwater), mFog->getFogEnd(isUnderwater));

        auto& sceneData = mSceneData->value();
        sceneData.fogColor = vsg::vec4(fogColor[0], fogColor[1], fogColor[2], 1.f);

        auto ambient = mAmbientColor;
        ambient += vsg::vec4(0.7, 0.7, 0.7, 0.0) * mNightEyeFactor;
        sceneData.ambient = ambient;
        sceneData.skyColor
            = mRenderModes[Render_Sky] ? mSky->getSkyColor() : mFog->getFogColor(false) * vsg::vec4(1.3, 1.3, 1.3, 1);
        sceneData.rainIntensity = mRenderModes[Render_Sky] ? mSky->getRainIntensity() : 0;
        sceneData.lightDiffuse = mLightDiffuse;
        sceneData.lightSpecular = mLightSpecular;
        sceneData.invView = vsg::mat4(vsg::inverse_4x3(mViewMatrix->transform()));
        mSceneData->setLightPosition(mLightPosition, sceneData.invView);
        sceneData.lightViewPos.a = mRenderModes[Render_Sky] ? mSky->getLightSize() : 8.f;

        const auto& vp = mSceneCamera->getViewport();
        sceneData.resolution = { vp.width, vp.height };
        mShadow->updateCascades(*mSceneCamera, sceneData, mLightPosition);
        mSceneData->dirty();
        mLightGrid->update();
    }

    void RenderManager::configureFog(const MWWorld::Cell& cell)
    {
        mFog->configure(mViewDistance, cell);
    }

    void RenderManager::configureFog(
        float fogDepth, float underwaterFog, float dlFactor, float dlOffset, const vsg::vec4& color)
    {
        mFog->configure(mViewDistance, fogDepth, underwaterFog, dlFactor, dlOffset, color);
    }

    void RenderManager::onFrame(float dt)
    {
        updateSceneData();
        mReflect->update();

        if (mRenderModes[Render_Terrain])
            updateTerrain(mViewMatrix->eye);
    }

    void RenderManager::setupScreenshotInterface()
    {
        mScreenshotInterface->request = [&](int w, int h) {
            auto device = mCompileContext->device;
            auto pair = Render::createDownloadCommands(device, mEngine.colorImageView(0)->image, { { 0, 0 }, mEngine.extent2D() },
                Render::compatibleColorFormat, mEngine.getPresentLayout(),
                VkExtent2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });
            auto commands = pair.first;
            auto dstImage = pair.second;
            mScreenshotSwitch->children = {{ true, commands }};
            mScreenshotInterface->framesUntilReady = mEngine.numFrames();
            mScreenshotInterface->retrieve = [this, device, dstImage]() -> std::pair<vsg::ref_ptr<vsg::Data>, bool> {
                mScreenshotSwitch->children.clear();
                if (--mScreenshotInterface->framesUntilReady > 0)
                    return { {}, false };
                auto out_data = vsg::ubvec4Array2D::create(
                    dstImage->extent.width, dstImage->extent.height, vsg::Data::Layout{ dstImage->format });
                if (!Render::mapAndCopy(device, dstImage, out_data))
                    out_data = {};
                return { out_data, true };
            };
        };
    }

    void RenderManager::update(float dt, bool paused)
    {
        if (paused)
            dt = 0; //return;
        else
            mSceneData->value().time += dt;

        mEffects->update(dt);
        mProjectiles->update(dt);
        if (mRenderModes[Render_Sky])
            mSky->update(dt);

        mPlayer->update(dt);
        setFirstPersonMode(mCamera->getMode() == Camera::Mode::FirstPerson);

        mScene->update(dt);
        /*
        updateNavMesh();
        updateRecastMesh();
        */
        mCamera->update(dt, paused);
        mCamera->updateCamera(*mViewMatrix);
    }

    void RenderManager::updatePlayerPtr(const MWWorld::Ptr& ptr)
    {
        if (mPlayer.get())
        {
            setupPlayer(ptr);
            mPlayer->updatePtr(ptr);
        }
        mCamera->attachTo(ptr);
    }

    void RenderManager::rotateObject(const MWWorld::Ptr& ptr, const osg::Quat& rot)
    {
        if (ptr == mCamera->getTrackingPtr() && !mCamera->isVanityOrPreviewModeEnabled())
            mCamera->rotateCameraToTrackingPtr();
        if (auto obj = getObject(ptr))
            obj->transform()->setAttitude(toVsg(rot));
    }

    void RenderManager::moveObject(const MWWorld::Ptr& ptr, const osg::Vec3f& pos)
    {
        if (auto obj = getObject(ptr))
            obj->transform()->translation = toVsg(pos);
    }

    void RenderManager::scaleObject(const MWWorld::Ptr& ptr, const osg::Vec3f& scale)
    {
        if (auto obj = getObject(ptr))
            obj->transform()->scale = toVsg(scale);
        if (ptr == mCamera->getTrackingPtr()) // update height of camera
            mCamera->processViewChange();
    }

    void RenderManager::removeObject(const MWWorld::Ptr& ptr)
    {
        /*
        mActorsPaths->remove(ptr);
        mWater->removeEmitter(ptr);
        */
        mScene->removeObject(ptr);
    }

    void RenderManager::setWaterEnabled(bool enabled)
    {
        setRenderModeActive(Render_Water, enabled);
        mSimpleWater->setEnabled(enabled);
    }

    void RenderManager::setWaterHeight(float height)
    {
        mWater->setHeight(height);
        mSimpleWater->setHeight(height);
    }

    osg::Vec4f RenderManager::getScreenBounds(const osg::BoundingBox& worldbb)
    {
        if (!worldbb.valid())
            return {};
        auto viewProj = mProjectionMatrix->transform() * mViewMatrix->transform();
        float min_x = 1.0f, max_x = 0.0f, min_y = 1.0f, max_y = 0.0f;
        for (int i = 0; i < 8; ++i)
        {
            auto screen = Render::worldToScreen(viewProj, vsg::dvec3(toVsg(worldbb.corner(i))));
            min_x = std::min(min_x, screen.x);
            max_x = std::max(max_x, screen.x);
            min_y = std::min(min_y, screen.y);
            max_y = std::max(max_y, screen.y);
        }
        return { min_x, min_y, max_x, max_y };
    }

    RenderManager::RayResult RenderManager::castRay(
        const osg::Vec3f& origin, const osg::Vec3f& dest, bool ignorePlayer, bool ignoreActors)
    {
        return MWRender::castRay(vsg::dvec3(toVsg(origin)), vsg::dvec3(toVsg(dest)), ignorePlayer, ignoreActors, *mScene, mIntersectorScene);
    }

    RenderManager::RayResult RenderManager::castCameraToViewportRay(
        float nX, float nY, float maxDistance, bool ignorePlayer, bool ignoreActors)
    {
        return MWRender::castCameraToViewportRay(nX, nY, maxDistance, ignorePlayer, ignoreActors, *mSceneCamera, *mScene, mIntersectorScene);
    }

    void RenderManager::updatePtr(const MWWorld::Ptr& old, const MWWorld::Ptr& updated)
    {
        mScene->updatePtr(old, updated);
        // mActorsPaths->updatePtr(old, updated);
    }

    void RenderManager::spawnEffect(const std::string& model, std::string_view texture, const osg::Vec3f& worldPosition,
        float scale, bool isMagicVFX)
    {
        mEffects->add(model, std::string(texture), toVsg(worldPosition), scale, isMagicVFX);
    }

    void RenderManager::notifyWorldSpaceChanged()
    {
        mEffects->clear();
        // mWater->clearRipples();
    }

    void RenderManager::clear()
    {
        mRenderModes.clear();
        mRenderModes.resize(Render_Count);
        updateSceneChildren();
        mSky->setMoonColour(false);

        notifyWorldSpaceChanged();
        /*
        if (mObjectPaging)
            mObjectPaging->clear();
            */
    }

    MWAnim::Object* RenderManager::getObject(const MWWorld::Ptr& ptr)
    {
        if (mPlayer.get() && ptr == mPlayer->getPtr())
            return mPlayer.get();
        return mScene->getObject(ptr);
    }

    const MWAnim::Object* RenderManager::getObject(const MWWorld::ConstPtr& ptr) const
    {
        const auto p
            = MWWorld::Ptr(const_cast<MWWorld::LiveCellRefBase*>(ptr.mRef), const_cast<MWWorld::CellStore*>(ptr.mCell));
        return const_cast<RenderManager*>(this)->getObject(p);
    }

    vsg::box RenderManager::getBoundingBox(const MWWorld::ConstPtr& ptr) const
    {
        auto obj = getObject(ptr);
        if (!obj)
            return {};
        return vsg::box(vsg::visit<vsg::ComputeBounds>(obj->transform()).bounds);
    }

    void RenderManager::setupPlayer(const MWWorld::Ptr& player)
    {
        /*
        mWater->removeEmitter(player);
        mWater->addEmitter(player);
        */
    }

    void RenderManager::renderPlayer(const MWWorld::Ptr& player)
    {
        mPlayer.reset(new Player(mScene->context, player, Npc::ViewMode::Normal));
        mPlayer->compile();
        mPlayerGroup->children = { { Mask_Player, mPlayer->node() } };
        mCamera->setAnimation(mPlayer.get());
        mCamera->attachTo(player);
    }

    void RenderManager::rebuildPtr(const MWWorld::Ptr& ptr)
    {
        if (Npc* anim = dynamic_cast<Npc*>(getObject(ptr)))
        {
            anim->rebuild();
            if (anim == mPlayer.get() && mCamera->getTrackingPtr() == ptr)
            {
                mCamera->attachTo(ptr);
                mCamera->setAnimation(mPlayer.get());
            }
        }
    }

    void RenderManager::addWaterRippleEmitter(const MWWorld::Ptr& ptr)
    {
        // mWater->addEmitter(ptr);
    }

    void RenderManager::removeWaterRippleEmitter(const MWWorld::Ptr& ptr)
    {
        // mWater->removeEmitter(ptr);
    }

    void RenderManager::emitWaterRipple(const osg::Vec3f& pos)
    {
        // mWater->emitRipple(pos);
    }

    void RenderManager::updateProjectionMatrix()
    {
        float fov = getFieldOfView();
        auto windowSize = mEngine.extent2D();
        *mProjectionMatrix = vsg::Perspective(fov,
            static_cast<double>(windowSize.width) / static_cast<double>(windowSize.height), mNearClip, mViewDistance);

        mSceneData->setDepthRange(mNearClip, mViewDistance);
        auto& sceneData = mSceneData->value();
        sceneData.projection = vsg::mat4(mProjectionMatrix->transform());
        sceneData.invProjection = vsg::inverse(sceneData.projection);
    }

    void RenderManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "Camera" && it->second == "field of view")
            {
                mFieldOfView = Settings::Manager::getFloat("field of view", "Camera");
                updateProjectionMatrix();
            }
            else if (it->first == "Camera" && it->second == "viewing distance")
            {
                mViewDistance = Settings::Manager::getFloat("viewing distance", "Camera");
                // mFog->configure
                updateProjectionMatrix();
            }
        }
    }

    float RenderManager::getTerrainHeightAt(const osg::Vec3f& pos)
    {
        return mTerrainStorage->getHeightAt(toVsg(pos));
    }

    void RenderManager::overrideFieldOfView(float val)
    {
        if (!mFieldOfViewOverride || *mFieldOfViewOverride != val)
        {
            mFieldOfViewOverride = val;
            updateProjectionMatrix();
        }
    }

    void RenderManager::setFieldOfView(float val)
    {
        if (val != mFieldOfView)
        {
            mFieldOfView = val;
            updateProjectionMatrix();
        }
    }

    float RenderManager::getFieldOfView() const
    {
        return mFieldOfViewOverride ? *mFieldOfViewOverride : mFieldOfView;
    }

    void RenderManager::resetFieldOfView()
    {
        if (mFieldOfViewOverride)
        {
            mFieldOfViewOverride = {};
            updateProjectionMatrix();
        }
    }

    void RenderManager::setViewDistance(float d, bool dummy)
    {
        if (mViewDistance != d)
        {
            mViewDistance = d;
            updateProjectionMatrix();
        }
    }

    osg::Vec3f RenderManager::getHalfExtents(const MWWorld::ConstPtr& object) const
    {
        std::string modelName = object.getClass().getModel(object);
        if (modelName.empty())
            return {};

        vsg::ref_ptr<const vsg::Node> node = vsgUtil::readNode(modelName, mResourceSystem->nodeOptions);
        auto bounds = vsg::visit<vsg::ComputeBounds>(node).bounds;
        if (bounds.valid())
        {
            auto half = vsg::vec3(vsgUtil::extent(bounds) / 2.0);
            return { half.x, half.y, half.z };
        }
        return {};
    }

    void RenderManager::exportSceneGraph(const MWWorld::Ptr& ptr, const std::filesystem::path& dir)
    {
        vsg::ref_ptr<vsg::Node> node = mSceneRoot;
        if (auto obj = getObject(ptr))
            node = obj->node();
        auto path = (dir / "scene.vsgt").string();
        if (!vsg::write(node, path))
            std::cerr << "!vsg::write(" << path << ")" << std::endl;
    }

    LandManager* RenderManager::getLandManager() const
    {
        return mTerrainStorage->getLandManager();
    }

    void RenderManager::removeActorPath(const MWWorld::ConstPtr& actor) const
    {
        // mActorsPaths->remove(actor);
    }

    void RenderManager::updateExtents(uint32_t w, uint32_t h)
    {
        mSceneCamera->viewportState->set(0, 0, w, h);
        mRenderGraph->renderArea = { { 0, 0 }, { w, h } };
        mGuiRenderGraph->renderArea = { { 0, 0 }, { w, h } };
        mEngine.updateExtents({ w, h });
    }

    vsg::ref_ptr<vsg::OperationThreads> RenderManager::getOperationThreads()
    {
        return mOperationThreads;
    }

    vsg::ref_ptr<vsgUtil::UpdateThreads> RenderManager::getUpdateThreads()
    {
        return mScene->getUpdateThreads();
    }

    void RenderManager::setFirstPersonMode(bool enabled)
    {
        if (enabled)
        {
            if (!mFirstPersonProjection)
            {
                auto firstPersonProjection = vsg::ref_ptr{ new vsgUtil::SetProjection };
                auto fov = vsg::ref_ptr{ new vsgUtil::FieldOfView };
                fov->perspective = mProjectionMatrix;
                fov->fieldOfViewY = std::clamp(Settings::Manager::getFloat("first person field of view", "Camera"), 1.f, 179.f);
                firstPersonProjection->projection = fov;
                mFirstPersonProjection = firstPersonProjection;
            }
            mPlayerGroup->children = {{ Mask_FirstPerson, mFirstPersonProjection }};
            mFirstPersonProjection->children = { mPlayer->node() };
        }
        else
            mPlayerGroup->children = {{ Mask_Player, mPlayer->node() }};
    }

    vsg::dmat4 RenderManager::getProjectionMatrix() const
    {
        return mProjectionMatrix->transform();
    }
}
