#include "rendermanager.hpp"

#include <limits>
#include <cstdlib>

#include <vsg/viewer/RenderGraph.h>
#include <vsg/viewer/CompileManager.h>
#include <vsg/commands/ExecuteCommands.h>
#include <vsg/vk/Context.h>
#include <vsg/io/write.h>
#include <vsg/traversals/ComputeBounds.h>

#include <components/pipeline/sets.hpp>
#include <components/pipeline/mode.hpp>
#include <components/vsgutil/readnode.hpp>
#include <components/vsgutil/box.hpp>
#include <components/vsgutil/intersectnormal.hpp>
#include <components/vsgutil/externalbin.hpp>
#include <components/vsgutil/sharedview.hpp>
#include <components/render/engine.hpp>
#include <components/view/defaultstate.hpp>
#include <components/view/lightgrid.hpp>
#include <components/view/collectlights.hpp>
#include <components/view/barriers.hpp>
#include <components/view/descriptors.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/animation/transform.hpp>
#include <components/mwanimation/object.hpp>
#include <components/mwanimation/color.hpp>
#include <components/terrain/grid.hpp>
#include <components/settings/settings.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/esm3/loadcell.hpp>
//#include <components/detournavigator/navigator.hpp>

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"

#include "mask.hpp"
#include "bin.hpp"
#include "terrainstorage.hpp"
#include "fogmanager.hpp"
#include "scene.hpp"
#include "camera.hpp"
#include "weather.hpp"
#include "player.hpp"
#include "effects.hpp"
#include "projectiles.hpp"
#include "env.hpp"
#include "shadow.hpp"

namespace MWRender
{
    RenderManager::RenderManager(Render::Engine &engine, vsg::Context *context, vsg::Node *gui, vsg::Node *guiRenderTextures, Resource::ResourceSystem* resourceSystem, const std::string& resourcePath/*, DetourNavigator::Navigator& navigator*/)
        : mEngine(engine)
        , mGui(gui)
        , mResourceSystem(resourceSystem)
        //, mNavigator(navigator)
        , mFog(new FogManager)

        , mMinimumAmbientLuminance(std::clamp(Settings::Manager::getFloat("minimum interior brightness", "Shaders"), 0.f, 1.f))
        , mNearClip(Settings::Manager::getFloat("near clip", "Camera"))
        , mViewDistance(Settings::Manager::getFloat("viewing distance", "Camera"))
        , mFieldOfView(std::clamp(Settings::Manager::getFloat("field of view", "Camera"), 1.f, 179.f))
        , mFirstPersonFieldOfView(std::clamp(Settings::Manager::getFloat("first person field of view", "Camera"), 1.f, 179.f))
    {
        mRenderModes.resize(Render_Count, true);
        mCamera.reset(new Camera);

        mShadow = std::make_unique<Shadow>(*context);
        mSky = std::make_unique<Sky>(*resourceSystem);

        int maxLights = 256;
        mDescriptors = std::make_unique<View::Descriptors>(maxLights, readEnv(resourceSystem->imageOptions), vsg::ref_ptr{mShadow->shadowMap()});
        mSceneRoot = View::createDefaultState(mDescriptors->getDescriptorSet());
        mPlayerGroup = vsg::Switch::create();

        mProjectionMatrix = vsg::Perspective::create();
        mViewMatrix = vsg::LookAt::create();
        mSceneCamera = vsg::Camera::create(mProjectionMatrix, mViewMatrix, vsg::ViewportState::create(mEngine.extent2D()));

        mCompileManager = mEngine.createCompileManager();
        mLightGrid = std::make_unique<View::LightGrid>(*resourceSystem->shaderOptions, mDescriptors->getDescriptorSet());
        mView = vsgUtil::createSharedView(mSceneCamera, mSceneRoot);
        mView->viewDependentState = new View::CollectLights(mDescriptors->lightDescriptor(), mDescriptors->lightData());
        mView->mask = ~Mask_GUI;
        mView->bins.push_back(vsg::Bin::create(Bin_DepthSorted, vsg::Bin::DESCENDING));

        mRenderGraph = mEngine.createRenderGraph();
        mRenderGraph->contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
        context->renderPass = mRenderGraph->getRenderPass();

        auto computeBin = vsg::ref_ptr{new vsgUtil::ExternalBin}; //VUID-vkCmdDispatch-renderpass
        computeBin->binNumber = Bin_Compute;
        mView->bins.push_back(computeBin);
        auto execParticles = vsg::ref_ptr{new vsgUtil::ExecuteBins};
        execParticles->bins = {computeBin};
        execParticles->mask = Mask_Particle;
        auto sceneGraph = mEngine.createSecondaryCommandGraph(mRenderGraph, Pipeline::TEXTURE_SET);
        sceneGraph->children = {mView};
        mRecordScene = sceneGraph->recordTraversal;

        auto guiGraph = mEngine.createSecondaryCommandGraph(mRenderGraph, 1);
        guiGraph->children = {mGui};

        auto exec = vsg::ExecuteCommands::create();
        exec->connect(sceneGraph);
        exec->connect(guiGraph);

        mRenderGraph->children = {exec};
        if (mRenderGraph->window)
            mCompileManager->add(*mRenderGraph->window, mView);
        else
            mCompileManager->add(*mRenderGraph->framebuffer, mView);

        auto threads = vsg::CommandGraphs{};

        auto castShadow = vsg::Group::create();
        if (auto &s = mShadow->shadow)
        {
            for (int i=0; i<s->numCascades; ++i)
            {
                auto rg = s->renderGraph(i);
                auto castShadowGraph = mEngine.createSecondaryCommandGraph(vsg::ref_ptr{rg}, Pipeline::TEXTURE_SET);
                auto view = s->cascadeView(i, ~(Mask_FirstPerson|Mask_Particle));
                view->bins = {vsg::Bin::create(Bin_DepthSorted, vsg::Bin::NO_SORT)};
                castShadowGraph->children = {view};
                auto exec = vsg::ExecuteCommands::create();
                exec->connect(castShadowGraph);
                rg->children = {exec};
                rg->contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
                castShadow->children.push_back(rg);
                threads.push_back(castShadowGraph);
                mRecordShadow.push_back(castShadowGraph->recordTraversal);
                if (i==0)
                    mCompileManager->add(*rg->framebuffer, view);
            }
        }
        threads.push_back(sceneGraph);
        threads.push_back(guiGraph);

        mCommandGraph = mEngine.createCommandGraph(Pipeline::COMPUTE_SET);
        mCommandGraph->camera = mSceneCamera; //lightGrid->setProjectionMatrix
        mCommandGraph->children = {context->copyImageCmd, mLightGrid->node(), execParticles, vsg::ref_ptr{guiRenderTextures}, castShadow, View::computeBarrier(), mRenderGraph};
        mRecord = mCommandGraph->recordTraversal;
        threads.push_back(mCommandGraph);

        mEngine.setup(threads);

        updateProjectionMatrix();

        auto normalMapPattern = Settings::Manager::getString("normal map pattern", "Shaders");
        auto heightMapPattern = Settings::Manager::getString("normal height map pattern", "Shaders");
        auto specularMapPattern = Settings::Manager::getString("terrain specular map pattern", "Shaders");
        auto useTerrainNormalMaps = Settings::Manager::getBool("auto use terrain normal maps", "Shaders");
        auto useTerrainSpecularMaps = Settings::Manager::getBool("auto use terrain specular maps", "Shaders");

        mTerrainStorage = std::make_unique<TerrainStorage>(mResourceSystem, normalMapPattern, heightMapPattern, useTerrainNormalMaps, specularMapPattern, useTerrainSpecularMaps);
        mTerrain = std::make_unique<Terrain::Grid>(mTerrainStorage.get(), mResourceSystem->imageOptions, mResourceSystem->shaderOptions);

        mScene = std::make_unique<Scene>(resourceSystem, mCompileManager);
        mEffects = std::make_unique<Effects>(mScene->context);
        mProjectiles = std::make_unique<Projectiles>(mScene->context);
        updateSceneChildren();
        createIntersectorScene();
    }

    RenderManager::~RenderManager()
    {
    }

    Scene &RenderManager::getObjects()
    {
        return *mScene;
    }

    vsg::ref_ptr<vsg::Node> RenderManager::createMapScene()
    {
        auto group = vsg::Group::create();
        group->children = {mTerrain->node(), mScene->getObjects()};//simpleWater
        return group;
    }

    vsg::ref_ptr<vsg::Node> RenderManager::createCastShadowScene(bool exterior)
    {
        auto group = vsg::Group::create();
        auto cat = "Shadows";
        if (!exterior && !Settings::Manager::getBool("enable indoor shadows", cat))
            return group;

        if (Settings::Manager::getBool("terrain shadows", cat))
            group->addChild(mTerrain->node());
        if (exterior && Settings::Manager::getBool("object shadows", cat))
            group->addChild(mScene->getObjects());
        if (Settings::Manager::getBool("actor shadows", cat))
            group->addChild(mScene->getActors());
        if (Settings::Manager::getBool("player shadows", cat))
            group->addChild(mPlayerGroup);
        return group;
    }

    void RenderManager::updateSceneChildren()
    {
        mSceneRoot->children.clear();
        if (mUseSky && mRenderModes[Render_Sky])
            mSceneRoot->addChild(mSky->node());
        if (!mRenderModes[Render_Scene])
            return;
        if (mUseTerrain)
            mSceneRoot->addChild(mTerrain->node());
        mSceneRoot->addChild(mPlayerGroup);
        mSceneRoot->addChild(mScene->getObjects());
        mSceneRoot->addChild(mScene->getActors());
        mSceneRoot->addChild(mProjectiles->node());
        mSceneRoot->addChild(mEffects->node());
    }

    void RenderManager::createIntersectorScene()
    {
        mIntersectorScene = vsg::Group::create();
        mIntersectorScene->children = {mTerrain->node(), mPlayerGroup, mScene->getObjects(), mScene->getActors()};
    }

    void RenderManager::setViewMode(ViewMode mode, bool enable)
    {
        vsg::Mask mask;
        if (mode == ViewMode::Scene)
            mask = ~Mask_GUI;
        //else if (mode == ViewMode::Gui)
        //    mask = Mask_GUI;

        for (auto traversal : std::initializer_list<vsg::RecordTraversal*>{mRecordScene, mRecord})
        {
            auto &traversalMask = traversal->traversalMask;
            if (enable)
                traversalMask |= mask;
            else
                traversalMask &= ~mask;
        }
        for (auto &traversal : mRecordShadow)
            traversal->traversalMask = enable ? mask : vsg::MASK_OFF;
    }

    bool RenderManager::toggleRenderMode(RenderMode mode)
    {
        bool enabled = mRenderModes[mode] = !mRenderModes[mode];
        updateSceneChildren();
        if (enabled)
            mEngine.compile();
        return enabled;
    }

    Resource::ResourceSystem* RenderManager::getResourceSystem()
    {
        return mResourceSystem;
    }

    /*
    Terrain::World* RenderManager::getTerrain()
    {
        return nullptr;//mTerrain.get();
    }
    */

    double RenderManager::getReferenceTime() const
    {
        return 0.0;//vsg::clock::now()
    }

    void RenderManager::setNightEyeFactor(float factor)
    {
        mNightEyeFactor = factor;
    }

    void RenderManager::setAmbientColour(const vsg::vec4 &colour)
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

    void RenderManager::configureAmbient(const ESM::Cell *cell)
    {
        auto ambient = vsg::vec4(MWAnim::rgbColor(cell->mAmbi.mAmbient),1.f);
        if (!cell->isExterior() && !(cell->mData.mFlags & ESM::Cell::QuasiEx))
            MWAnim::brighten(ambient, mMinimumAmbientLuminance);
        setAmbientColour(ambient);

        auto diffuse = vsg::vec4(MWAnim::rgbColor(cell->mAmbi.mSunlight),1.f);
        mLightDiffuse = diffuse;
        mLightSpecular = diffuse;
        mLightPosition = vsg::vec3(-0.15f, 0.15f, 1.f);
    }

    void RenderManager::setSunColour(const osg::Vec4f& diffuse, const osg::Vec4f& specular)
    {
        mLightDiffuse = toVsg(diffuse);
        mLightSpecular = toVsg(specular);
    }

    void RenderManager::setSunDirection(const osg::Vec3f &direction)
    {
        osg::Vec3 position = direction * -1;
        mLightPosition = toVsg(position);
        mSky->setSunDirection(position);
    }

    void RenderManager::addCell(const MWWorld::CellStore *store)
    {
        /*
        mPathgrid->addCell(store);
        mWater->changeCell(store);
        */
        if (store->getCell()->isExterior())
            mTerrain->loadCell(store->getCell()->getGridX(), store->getCell()->getGridY());
    }
    void RenderManager::removeCell(const MWWorld::CellStore *store)
    {
        /*
        mPathgrid->removeCell(store);
        mActorsPaths->removeCell(store);
        */
        mScene->removeCell(store);
        if (store->getCell()->isExterior())
            mTerrain->unloadCell(store->getCell()->getGridX(), store->getCell()->getGridY());

        //mWater->removeCell(store);
    }

    void RenderManager::enableTerrain(bool enable)
    {
        mUseTerrain = enable;
        updateSceneChildren();
    }

    Sky* RenderManager::getSky()
    {
        return mSky.get();
    }

    void RenderManager::setSkyEnabled(bool enabled)
    {
        mUseSky = enabled;
        updateSceneChildren();
        if (auto &s = mShadow->shadow)
            s->setCastShadowScene(createCastShadowScene(enabled));
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

    void RenderManager::updateSceneData(float dt)
    {
        bool isUnderwater = 0;//mWater->isUnderwater(mCamera->getPosition());
        auto fogColor = mFog->getFogColor(isUnderwater);
        mRenderGraph->setClearValues(VkClearColorValue{{fogColor.x,fogColor.y,fogColor.z, 1}});

        auto &sceneData = mDescriptors->sceneData();
        sceneData.setFogRange(mFog->getFogStart(isUnderwater), mFog->getFogEnd(isUnderwater));
        sceneData.fogColor = vsg::vec4(fogColor[0], fogColor[1], fogColor[2], 1.f);

        auto ambient = mAmbientColor;
        if (mNightEyeFactor > 0.f)
            ambient += vsg::vec4(0.7, 0.7, 0.7, 0.0) * mNightEyeFactor;
        sceneData.ambient = ambient;
        sceneData.lightDiffuse = mLightDiffuse;
        sceneData.lightSpecular = mLightSpecular;
        sceneData.time += dt;
        const auto &vp = mSceneCamera->getViewport();
        sceneData.resolution = {vp.width, vp.height};
        mDescriptors->setLightPosition(mLightPosition, *mSceneCamera);
        mShadow->updateCascades(*mSceneCamera, sceneData, mLightPosition);
        mDescriptors->copyDataListToBuffers();
        mLightGrid->update();
    }

    void RenderManager::configureFog(const ESM::Cell *cell)
    {
        mFog->configure(mViewDistance, cell);
    }

    void RenderManager::configureFog(float fogDepth, float underwaterFog, float dlFactor, float dlOffset, const vsg::vec4 &color)
    {
        mFog->configure(mViewDistance, fogDepth, underwaterFog, dlFactor, dlOffset, color);
    }

    void RenderManager::onFrame(float dt)
    {
        static_cast<View::CollectLights*>(mView->viewDependentState.get())->advanceFrame();
        //vsgopenmw-render-onframe
        updateSceneData(dt);
    }

    void RenderManager::update(float dt, bool paused)
    {
        /*
        float rainIntensity = mSky->getPrecipitationAlpha();
        mWater->setRainIntensity(rainIntensity);
        */
        if (!paused)
        {
            mEffects->update(dt);
            mProjectiles->update(dt);
            mSky->update(dt);
            /*
            mWater->update(dt);

            const MWWorld::Ptr& player = mPlayer->getPtr();
            osg::Vec3f playerPos(player.getRefData().getPosition().asVec3());

            float windSpeed = mSky->getBaseWindSpeed();
            mSharedUniformStateUpdater->setWindSpeed(windSpeed);
            mSharedUniformStateUpdater->setPlayerPos(playerPos);
            */
        }

        mPlayer->update(dt);
        mPlayerGroup->children[0].mask = mCamera->getMode() == Camera::Mode::FirstPerson ? Mask_FirstPerson : Mask_Player;

        mScene->update(dt);
        /*
        updateNavMesh();
        updateRecastMesh();
        */
        mCamera->update(dt, paused);
        mCamera->updateCamera(*mViewMatrix);
    }

    void RenderManager::updatePlayerPtr(const MWWorld::Ptr &ptr)
    {
        if(mPlayer.get())
        {
            setupPlayer(ptr);
            mPlayer->updatePtr(ptr);
        }
        mCamera->attachTo(ptr);
    }

    void RenderManager::removePlayer(const MWWorld::Ptr &player)
    {
        //mWater->removeEmitter(player);
    }

    void RenderManager::rotateObject(const MWWorld::Ptr &ptr, const osg::Quat& rot)
    {
        if(ptr == mCamera->getTrackingPtr() && !mCamera->isVanityOrPreviewModeEnabled())
            mCamera->rotateCameraToTrackingPtr();
        if (auto obj = getObject(ptr))
            obj->transform()->setAttitude(toVsg(rot));
    }

    void RenderManager::moveObject(const MWWorld::Ptr &ptr, const osg::Vec3f &pos)
    {
        if (auto obj = getObject(ptr))
            obj->transform()->translation = toVsg(pos);
    }

    void RenderManager::scaleObject(const MWWorld::Ptr &ptr, const osg::Vec3f &scale)
    {
        if (auto obj = getObject(ptr))
            obj->transform()->scale = toVsg(scale);
        if (ptr == mCamera->getTrackingPtr()) // update height of camera
            mCamera->processViewChange();
    }

    void RenderManager::removeObject(const MWWorld::Ptr &ptr)
    {
        /*
        mActorsPaths->remove(ptr);
        mScene->removeObject(ptr);
        mWater->removeEmitter(ptr);
        */
    }

    void RenderManager::setWaterEnabled(bool enabled)
    {
        //mWater->setEnabled(enabled);
        mSky->setWaterEnabled(enabled);
    }

    void RenderManager::setWaterHeight(float height)
    {
        /*
        mWater->setHeight(height);
        */
        mSky->setWaterHeight(height);
    }
/*
    void RenderManager::screenshot(osg::Image* image, int w, int h)
    {
        //mScreenshotManager->screenshot(image, w, h);
    }
*/

    osg::Vec4f RenderManager::getScreenBounds(const osg::BoundingBox &worldbb)
    {
        if (!worldbb.valid()) return {};
        auto viewProj = mProjectionMatrix->transform() * mViewMatrix->transform();
        float min_x = 1.0f, max_x = 0.0f, min_y = 1.0f, max_y = 0.0f;
        for (int i=0; i<8; ++i)
        {
            auto corner = viewProj * vsg::dvec3(toVsg(worldbb.corner(i)));
            float x = (corner.x + 1.f) * 0.5f;
            float y = (corner.y + 1.f) * 0.5f;
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
            min_y = std::min(min_y, y);
            max_y = std::max(max_y, y);
        }
        return {min_x, min_y, max_x, max_y};
    }

    RenderManager::RayResult getIntersectionResult (vsg::LineSegmentIntersector &intersector, const vsg::dvec3 &eye, Scene &scene, std::optional<float> maxRatio)
    {
        RenderManager::RayResult result;
        auto intersections = intersector.intersections;
        std::sort(intersections.begin(), intersections.end(), [](auto &lhs, auto &rhs) -> auto { return lhs->ratio < rhs->ratio; });
        for (auto &intersection : intersections)
        {
            if (maxRatio && intersection->ratio >= *maxRatio)
                continue;
            auto w = intersection->worldIntersection;
            result.mHitPointWorld = osg::Vec3f(w.x, w.y, w.z);
            vsg::vec3 nWorld = vsgUtil::worldNormal(*intersection, static_cast<int>(Pipeline::GeometryMode::NORMAL));
            result.mHitNormalWorld = {nWorld.x, nWorld.y, nWorld.z};
            auto dir = vsg::normalize(eye - vsg::dvec3(toVsg(result.mHitPointWorld)));
            if (vsg::length2(nWorld) > 0 && vsg::dot(dir, vsg::dvec3(nWorld)) < 0)
                continue;

            result.mRatio = intersection->ratio;
            for (auto node : intersection->nodePath)
            {
                if (auto trans = dynamic_cast<const Anim::Transform*>(node))
                {
                    result.mHitObject = scene.getPtr(trans);
                    return result;
                }
            }
            break;
        }
        return result;
    }

    void configure(vsg::Intersector &visitor, bool ignorePlayer, bool ignoreActors)
    {
        visitor.traversalMask = ~Mask_FirstPerson;
        if (ignoreActors)
            visitor.traversalMask &= ~(Mask_Actor);
        if (ignorePlayer)
            visitor.traversalMask &= ~(Mask_Player);
    }

    RenderManager::RayResult RenderManager::castRay(const osg::Vec3f& origin, const osg::Vec3f& dest, bool ignorePlayer, bool ignoreActors)
    {
        auto intersector = vsg::LineSegmentIntersector(vsg::dvec3(toVsg(origin)), vsg::dvec3(toVsg(dest)));
        configure(intersector, ignorePlayer, ignoreActors);
        return getIntersectionResult(vsg::visit(intersector, mIntersectorScene), mViewMatrix->eye, *mScene, {});
    }

    RenderManager::RayResult RenderManager::castCameraToViewportRay(float nX, float nY, float maxDistance, bool ignorePlayer, bool ignoreActors)
    {
        const auto &vp = mSceneCamera->getViewport();
        auto intersector = vsg::LineSegmentIntersector(*mSceneCamera, static_cast<int32_t>(nX*vp.width), static_cast<int32_t>(nY*vp.height));
        configure(intersector, ignorePlayer, ignoreActors);
        return getIntersectionResult(vsg::visit(intersector, mIntersectorScene), mViewMatrix->eye, *mScene, {maxDistance / mViewDistance});
    }

    void RenderManager::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        mScene->updatePtr(old, updated);
        //mActorsPaths->updatePtr(old, updated);
    }

    void RenderManager::spawnEffect(const std::string &model, const std::string &texture, const osg::Vec3f &worldPosition, float scale, bool isMagicVFX)
    {
        mEffects->add(model, texture, toVsg(worldPosition), scale, isMagicVFX);
    }

    void RenderManager::notifyWorldSpaceChanged()
    {
        mEffects->clear();
        //mWater->clearRipples();
    }

    void RenderManager::clear()
    {
        mRenderModes.clear();
        mRenderModes.resize(Render_Count, true);
        updateSceneChildren();
        mSky->setMoonColour(false);

        /*
        notifyWorldSpaceChanged();
        if (mObjectPaging)
            mObjectPaging->clear();
            */
    }

    MWAnim::Object *RenderManager::getObject(const MWWorld::Ptr& ptr)
    {
        if (mPlayer.get() && ptr == mPlayer->getPtr())
            return mPlayer.get();
        return mScene->getObject(ptr);
    }

    const MWAnim::Object *RenderManager::getObject(const MWWorld::ConstPtr &ptr) const
    {
        const auto p = MWWorld::Ptr(const_cast<MWWorld::LiveCellRefBase*>(ptr.mRef), const_cast<MWWorld::CellStore*>(ptr.mCell));
        return const_cast<RenderManager*>(this)->getObject(p);
    }

    vsg::box RenderManager::getBoundingBox(const MWWorld::ConstPtr& ptr) const
    {
        auto obj = getObject(ptr);
        if (!obj)
            return {};
        vsg::ComputeBounds visitor;
        obj->transform()->accept(visitor);
        return vsg::box(visitor.bounds);
    }

    void RenderManager::setupPlayer(const MWWorld::Ptr &player)
    {
        /*
        mWater->removeEmitter(player);
        mWater->addEmitter(player);
        */
    }

    void RenderManager::renderPlayer(const MWWorld::Ptr &player)
    {
        if (mPlayer.get()) return;//; vsgopenmw-deletion-queue
        mPlayer.reset(new Player(mScene->context, player, Npc::ViewMode::Normal, mFirstPersonFieldOfView));
        mPlayerGroup->children = {{Mask_Player, mPlayer->node()}};
        mCamera->setAnimation(mPlayer.get());
        mCamera->attachTo(player);
    }

    void RenderManager::rebuildPtr(const MWWorld::Ptr &ptr)
    {
        if (Npc *anim = dynamic_cast<Npc*>(getObject(ptr)))
        {
            anim->rebuild();
            if(anim == mPlayer.get() && mCamera->getTrackingPtr() == ptr)
            {
                mCamera->attachTo(ptr);
                mCamera->setAnimation(mPlayer.get());
            }
        }
    }

    void RenderManager::addWaterRippleEmitter(const MWWorld::Ptr &ptr)
    {
        //mWater->addEmitter(ptr);
    }

    void RenderManager::removeWaterRippleEmitter(const MWWorld::Ptr &ptr)
    {
        //mWater->removeEmitter(ptr);
    }

    void RenderManager::emitWaterRipple(const osg::Vec3f &pos)
    {
        //mWater->emitRipple(pos);
    }

    void RenderManager::updateProjectionMatrix()
    {
        float fov = getFieldOfView();
        auto windowSize = mEngine.extent2D();
        *mProjectionMatrix = vsg::Perspective(fov, static_cast<double>(windowSize.width) / static_cast<double>(windowSize.height), mNearClip, mViewDistance);
        mDescriptors->sceneData().setDepthRange(mNearClip, mViewDistance);
    }

    void RenderManager::processChangedSettings(const Settings::CategorySettingVector &changed)
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
                //mFog->configure
                updateProjectionMatrix();
            }
            else if (it->first == "Water")
            {
                ;//mWater->processChangedSettings(changed);
            }
        }
    }

    float RenderManager::getTerrainHeightAt(const osg::Vec3f &pos)
    {
        return mTerrain->getHeightAt(toVsg(pos));
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
            auto half = vsg::vec3(vsgUtil::extent(bounds)/2.0);
            return {half.x, half.y, half.z};
        }
        return {};
    }

    void RenderManager::exportSceneGraph(const MWWorld::Ptr &ptr, const std::string &folder)
    {
        vsg::ref_ptr<vsg::Node> node = mSceneRoot;
        if (auto obj = getObject(ptr))
            node = obj->node();
        vsg::write(node, folder + "/scene.vsgt");
    }

    LandManager *RenderManager::getLandManager() const
    {
        return mTerrainStorage->getLandManager();
    }
/*
    void RenderManager::updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
            const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end) const
    {
        mActorsPaths->update(actor, path, halfExtents, start, end, mNavigator.getSettings());
    }
    */

    void RenderManager::removeActorPath(const MWWorld::ConstPtr& actor) const
    {
        //mActorsPaths->remove(actor);
    }
/*
    void RenderManager::setNavMeshNumber(const std::size_t value)
    {
        mNavMeshNumber = value;
    }

    void RenderManager::updateNavMesh()
    {
        if (!mNavMesh->isEnabled())
            return;

        const auto navMeshes = mNavigator.getNavMeshes();

        auto it = navMeshes.begin();
        for (std::size_t i = 0; it != navMeshes.end() && i < mNavMeshNumber; ++i)
            ++it;
        if (it == navMeshes.end())
        {
            mNavMesh->reset();
        }
        else
        {
            try
            {
                const auto locked = it->second->lockConst();
                mNavMesh->update(locked->getImpl(), mNavMeshNumber, locked->getGeneration(),
                                 locked->getNavMeshRevision(), mNavigator.getSettings());
            }
            catch (const std::exception& e)
            {
                std::cerr << "NavMesh render update exception: " << e.what() << std::endl;
            }
        }
    }

    void RenderManager::updateRecastMesh()
    {
        if (!mRecastMesh->isEnabled())
            return;

        mRecastMesh->update(mNavigator.getRecastMeshTiles(), mNavigator.getSettings());
    }

    void RenderManager::setActiveGrid(const osg::Vec4i &grid)
    {
        mTerrain->setActiveGrid(grid);
    }
    bool RenderManager::pagingEnableObject(int type, const MWWorld::ConstPtr& ptr, bool enabled)
    {
        if (!ptr.isInCell() || !ptr.getCell()->isExterior() || !mObjectPaging)
            return false;
        if (mObjectPaging->enableObject(type, ptr.getCellRef().getRefNum(), ptr.getCellRef().getPosition().asVec3(), osg::Vec2i(ptr.getCell()->getCell()->getGridX(), ptr.getCell()->getCell()->getGridY()), enabled))
        {
            mTerrain->rebuildViews();
            return true;
        }
        return false;
    }
    void RenderManager::pagingBlacklistObject(int type, const MWWorld::ConstPtr &ptr)
    {
        if (!ptr.isInCell() || !ptr.getCell()->isExterior() || !mObjectPaging)
            return;
        const ESM::RefNum & refnum = ptr.getCellRef().getRefNum();
        if (!refnum.hasContentFile()) return;
        if (mObjectPaging->blacklistObject(type, refnum, ptr.getCellRef().getPosition().asVec3(), osg::Vec2i(ptr.getCell()->getCell()->getGridX(), ptr.getCell()->getCell()->getGridY())))
            mTerrain->rebuildViews();
    }
    bool RenderManager::pagingUnlockCache()
    {
        if (mObjectPaging && mObjectPaging->unlockCache())
        {
            mTerrain->rebuildViews();
            return true;
        }
        return false;
    }
    void RenderManager::getPagedRefnums(const osg::Vec4i &activeGrid, std::set<ESM::RefNum> &out)
    {
        if (mObjectPaging)
            mObjectPaging->getPagedRefnums(activeGrid, out);
    }
    */

    void RenderManager::setCompileRequired(bool compile)
    {
        if (mScene->compile != compile) //vsgopenmw-thread-safety
            mScene->compile = compile;
    }

    void RenderManager::updateExtents(uint32_t w, uint32_t h)
    {
        mSceneCamera->viewportState->set(0,0,w,h);
        mRenderGraph->renderArea = {{0,0},{w,h}};
        mEngine.updateExtents({w,h});
    }
}
