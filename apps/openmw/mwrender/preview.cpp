#include "preview.hpp"

#include <iostream>

#include <vsg/viewer/CompileManager.h>
#include <vsg/viewer/Camera.h>
#include <vsg/viewer/View.h>
#include <vsg/nodes/Switch.h>
#include <vsg/traversals/LineSegmentIntersector.h>
#include <vsg/io/Options.h>
#include <vsg/state/DynamicState.h>

#include <components/fallback/fallback.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/animation/transform.hpp>
#include <components/mwanimation/play.hpp>
#include <components/mwanimation/groups.hpp>
#include <components/view/collectlights.hpp>
#include <components/view/defaultstate.hpp>
#include <components/view/descriptors.hpp>
#include <components/vsgutil/computetransform.hpp>
#include <components/vsgutil/nullbin.hpp>
#include <components/vsgutil/sharedview.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/render/rendertexture.hpp>
#include <components/pipeline/override.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "npc.hpp"
#include "env.hpp"
#include "mask.hpp"
#include "animcontext.hpp"
#include "bin.hpp"

namespace
{
    void pipelineOptions(Pipeline::Options &o)
    {
        // I *think* (based on some by-hand maths) that the RGB and dest alpha factors are unchanged, and only dest determines source alpha factor
        if (o.srcBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
            o.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        else if (o.srcBlendFactor == VK_BLEND_FACTOR_ONE)
            o.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        /*else
            std::cerr << "o.srcBlendFactor " << o.srcBlendFactor<< std::endl;*/
    }

    // Set up alpha blending mode to avoid issues caused by transparent objects writing onto the alpha value of the FBO
    // This makes the RTT have premultiplied alpha, though, so the source blend factor must be GL_ONE when it's applied
    vsg::ref_ptr<vsg::Options> premultAlphaOptions (const vsg::Options &inOptions)
    {
        auto options = vsg::Options::create(inOptions);
        auto override_ = vsg::ref_ptr{new Pipeline::Override};
        override_->composite(pipelineOptions, *options);
        return options;
    }
}
namespace MWRender
{
    Preview::Preview(vsg::Context &ctx, vsg::CompileManager *compile, Resource::ResourceSystem* resourceSystem)
        : mCompile(compile)
    {
        mContext = std::make_unique<MWAnim::Context>(animContext(resourceSystem, compile, premultAlphaOptions(*resourceSystem->nodeOptions)));
        auto extent = VkExtent2D{textureWidth,textureHeight};
        constexpr float fovY = 12.3f;
        constexpr float near = 4.0f;
        constexpr float far = 10000.f;
        mPerspective = vsg::Perspective::create(fovY, static_cast<double>(textureWidth) / static_cast<double>(textureHeight), near, far);
        mLookAt = vsg::LookAt::create();
        camera = vsg::Camera::create(mPerspective, mLookAt, vsg::ViewportState::create(extent));
        mDescriptors = std::make_unique<View::Descriptors>(1, readEnv(resourceSystem->imageOptions));
        mScene = View::createDefaultState(mDescriptors->getDescriptorSet());

        float azimuth = osg::DegreesToRadians(Fallback::Map::getFloat("Inventory_DirectionalRotationX"));
        float altitude = osg::DegreesToRadians(Fallback::Map::getFloat("Inventory_DirectionalRotationY"));
        mLightPos = {-std::cos(azimuth) * std::sin(altitude),
            std::sin(azimuth) * std::sin(altitude),
            std::cos(altitude)};

        mView = vsgUtil::createSharedView(camera, mScene);
        mView->viewDependentState = new View::CollectLights(mDescriptors->lightDescriptor(), mDescriptors->lightData());
        mView->mask = ~Mask_GUI;
        mView->bins = {
            vsg::Bin::create(Bin_DepthSorted, vsg::Bin::DESCENDING),
            vsgUtil::NullBin::create(Bin_Compute)
        };

        auto &sceneData = mDescriptors->sceneData();
        sceneData.lightDiffuse = {Fallback::Map::getFloat("Inventory_DirectionalDiffuseR"),
            Fallback::Map::getFloat("Inventory_DirectionalDiffuseG"),
            Fallback::Map::getFloat("Inventory_DirectionalDiffuseB"), 1};
        sceneData.ambient = {Fallback::Map::getFloat("Inventory_DirectionalAmbientR"),
            Fallback::Map::getFloat("Inventory_DirectionalAmbientG"),
            Fallback::Map::getFloat("Inventory_DirectionalAmbientB"), 1};

        auto renderGraph = Render::createRenderTexture(ctx, extent, mTexture);
        renderGraph->setClearValues(VkClearColorValue{{0,0,0,0}});
        renderGraph->children = {mView};

        compile->add(*renderGraph->framebuffer, mView);
        mNode = vsg::Switch::create();
        mNode->children = {{false, renderGraph}};
    }

    Preview::~Preview()
    {
    }

    void Preview::compile()
    {
        if (mContext->compile(npc->node()))
            mScene->children = {npc->node()};
    }

    void Preview::rebuild(const MWWorld::Ptr &character, bool headOnly)
    {
        mScene->children.clear();
        //vsgopenmw-deletion-queue
        //if (npc) deletionQueue. 
        try
        {
            npc = std::make_unique<MWRender::Npc>(*mContext, character, headOnly ? Npc::ViewMode::HeadOnly : Npc::ViewMode::Normal);
            npc->show(MWAnim::Wielding::Wield::Weapon, true);
            compile();
        }
        catch (std::exception &e)
        {
            npc.reset();
            std::cerr << "!Preview::rebuild(" << e.what() << ")" << std::endl;
        }
    }

    void Preview::setViewport(uint32_t w, uint32_t h)
    {
        w = std::min(textureWidth, w);
        h = std::min(textureHeight, h);
        camera->viewportState->set(0, 0, w, h);
        mPerspective->aspectRatio = static_cast<double>(w)/static_cast<double>(h);
        redraw();
    }

    void Preview::animate()
    {
        if (npc)
        {
            npc->animation->runAnimation(0);
            npc->update(0);
        }
    }

    void Preview::redraw()
    {
        if (npc && !mNeedRedraw)
            mNeedRedraw = true;
    }

    void Preview::onFrame()
    {
        mNode->setAllChildren(mNeedRedraw);
        mNeedRedraw = false;
    }

    void Preview::updateViewMatrix(const vsg::dvec3 &eye, const vsg::dvec3 &center)
    {
        *mLookAt = vsg::LookAt(eye, center, vsg::dvec3{0,0,1});
        mDescriptors->setLightPosition(mLightPos, *camera);
        mDescriptors->copyDataListToBuffers();
    }

    vsg::ImageView *Preview::getTexture()
    {
        return mTexture;
    }

    Inventory::Inventory(Preview *p, const MWWorld::Ptr& character)
        : preview(p)
    {
        rebuild(character);
    }

    void Inventory::update()
    {
        auto &npc = preview->npc;
        if (!npc)
            return;
        npc->updateEquipment();

        auto &animation = npc->animation;

        auto ptr = npc->getPtr();
        auto &inv = ptr.getClass().getInventoryStore(ptr);
        auto iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        std::string groupname = "inventoryhandtohand";
        bool showCarriedLeft = true;
        if(iter != inv.end())
        {
            groupname = "inventoryweapononehand";
            if(iter->getType() == ESM::Weapon::sRecordId)
            {
                MWWorld::LiveCellRef<ESM::Weapon> *ref = iter->get<ESM::Weapon>();
                int type = ref->mBase->mData.mType;
                const ESM::WeaponType* weaponInfo = MWMechanics::getWeaponType(type);
                showCarriedLeft = !(weaponInfo->mFlags & ESM::WeaponType::TwoHanded);

                std::string inventoryGroup = weaponInfo->mLongGroup;
                inventoryGroup = "inventory" + inventoryGroup;

                // We still should use one-handed animation as fallback
                if (animation->hasAnimation(inventoryGroup))
                    groupname = inventoryGroup;
                else
                {
                    static const std::string oneHandFallback = "inventory" + MWMechanics::getWeaponType(ESM::Weapon::LongBladeOneHand)->mLongGroup;
                    static const std::string twoHandFallback = "inventory" + MWMechanics::getWeaponType(ESM::Weapon::LongBladeTwoHand)->mLongGroup;

                    // For real two-handed melee weapons use 2h swords animations as fallback, otherwise use the 1h ones
                    if (weaponInfo->mFlags & ESM::WeaponType::TwoHanded && weaponInfo->mWeaponClass == ESM::WeaponType::Melee)
                        groupname = twoHandFallback;
                    else
                        groupname = oneHandFallback;
                }
           }
        }

        npc->show(MWAnim::Wielding::Wield::CarriedLeft, showCarriedLeft);

        animation->play(groupname, 1, MWAnim::BlendMask_All, false, 1.0f, "start", "stop", 0.0f, 0);

        auto torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if(torch != inv.end() && torch->getType() == ESM::Light::sRecordId && showCarriedLeft)
        {
            if(!animation->getInfo("torch"))
                animation->play("torch", 2, MWAnim::BlendMask_LeftArm, false,
                                 1.0f, "start", "stop", 0.0f, ~0ul, true);
        }
        else if(animation->getInfo("torch"))
            animation->disable("torch");
        preview->animate();
        preview->redraw();
    }

    int Inventory::getSlotSelected (int posX, int posY)
    {
        if (auto &npc = preview->npc)
        {
            auto intersector = vsg::LineSegmentIntersector(*preview->camera, posX, posY);
            npc->node()->accept(intersector);
            auto intersections = intersector.intersections;
            std::sort(intersections.begin(), intersections.end(), [](auto &lhs, auto &rhs) -> auto { return lhs->ratio < rhs->ratio; });
            for (auto &intersection : intersections)
            {
                return npc->getSlot(intersection->nodePath);
            }
        }
        return -1;
    }

    void Inventory::rebuild(const MWWorld::Ptr &ptr)
    {
        auto p = MWWorld::Ptr(ptr.getBase(), nullptr);
        preview->rebuild(p);
        osg::Vec3f scale (1.f, 1.f, 1.f);
        p.getClass().adjustScale(p, scale, true);
        if (preview->npc)
            preview->npc->transform()->scale = toVsg(scale);

        vsg::vec3 position {0,700,71};
        vsg::vec3 lookAt {0,0,71};
        preview->updateViewMatrix(vsg::dvec3(position * scale.z()), vsg::dvec3(lookAt * scale.z()));
    }

    RaceSelection::RaceSelection(Preview *p)
        : mRef(&mBase)
        , mPitchRadians(vsg::radians(6.f))
        , preview(p)
    {
    }

    void RaceSelection::setAngle(float angleRadians)
    {
        if (preview->npc)
            preview->npc->transform()->setAttitude(vsg::quat(mPitchRadians, {1,0,0}) * vsg::quat(angleRadians, {0,0,1}));
        preview->redraw();
    }

    void RaceSelection::setPrototype(const ESM::NPC &proto)
    {
        mBase = proto;

        preview->rebuild(MWWorld::Ptr(&mRef, nullptr), true);

        if (auto &npc = preview->npc)
        {
            npc->animation->play("idle", 1, MWAnim::BlendMask_All, false, 1.0f, "start", "stop", 0.0f, 0);
            preview->animate();

            if (auto head = npc->getHead())
            {
                auto headPos = vsgUtil::computePosition(npc->worldTransform(*head));
                vsg::vec3 positionOffset {0,125,8};
                vsg::vec3 centerOffset {0,0,8};
                preview->updateViewMatrix(vsg::dvec3(headPos + positionOffset), vsg::dvec3(headPos + centerOffset));
            }
            preview->redraw();
        }
    }
}
