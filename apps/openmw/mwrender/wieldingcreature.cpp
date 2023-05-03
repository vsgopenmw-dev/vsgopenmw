#include "wieldingcreature.hpp"

#include <components/animation/context.hpp>
#include <components/mwanimation/play.hpp>
#include <components/mwanimation/clone.hpp>
#include <components/mwanimation/context.hpp>
#include <components/mwanimation/decorate.hpp>
#include <components/mwanimation/attach.hpp>
#include <components/vsgutil/removechild.hpp>
#include <components/vsgutil/compilecontext.hpp>

#include "../mwmechanics/weapontype.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "env.hpp"

namespace MWRender
{

    WieldingCreature::WieldingCreature(const MWAnim::Context& mwctx)
        : Wielding(mwctx)
    {
    }

    void WieldingCreature::setup(const MWWorld::Ptr& ptr)
    {
        mPtr = ptr;
        addParts();
    }

    void WieldingCreature::addParts()
    {
        addPart(mPtr, MWWorld::InventoryStore::Slot_CarriedRight, mContext);
        addPart(mPtr, MWWorld::InventoryStore::Slot_CarriedLeft, mContext);
        addAmmo(mPtr, mContext);
    }

    void WieldingCreature::removeParts()
    {
        mStaticControllers = {};
        autoPlay = {};
        mWeaponControllers = {};

        for (int i = 0; i < 3; ++i)
        {
            auto& path = mParts[i];
            if (!path.empty())
            {
                mContext.compileContext->detach(vsg::ref_ptr{path.back()});
                //vsgUtil::prune<vsg::Group>(path);
                if (auto group = (*(path.rbegin()+1))->cast<vsg::Group>())
                    vsgUtil::removeChild(group, path.back());
                path.clear();
            }
        }
    }

    void WieldingCreature::update(float dt)
    {
        Wielding::update(dt);
    }

    void WieldingCreature::addPart(const MWWorld::Ptr& ptr, int slot, const MWAnim::Context& mwctx)
    {
        const MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
        MWWorld::ConstContainerStoreIterator it = inv.getSlot(slot);

        if (it == inv.end())
            return;

        MWWorld::ConstPtr item = *it;

        std::string bonename;
        /*const&*/ std::string model = item.getClass().getModel(item);
        if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
        {
            if (item.getType() == ESM::Weapon::sRecordId)
            {
                int type = item.get<ESM::Weapon>()->mBase->mData.mType;
                bonename = MWMechanics::getWeaponType(type)->mAttachBone;
                if (bonename != "Weapon Bone")
                {
                    auto b = animation->bones.search(bonename);
                    if (!b)
                        bonename = "Weapon Bone";
                }
            }
            else
                bonename = "Weapon Bone";
        }
        else
        {
            bonename = "Shield Bone";
            // if (item.getType() == ESM::Armor::sRecordId)
            // itemModel = getShieldMesh(item, false);
        }

        auto node = mwctx.readNode(model);
        auto result = MWAnim::decorate(node, {});
        addEnv(node, item);
        if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
        {
            mAttachAmmo = result.placeholders.attachAmmo;
            addSwitch(node, Wield::Weapon);
        }
        else
            addSwitch(node, Wield::CarriedLeft);

        int i = (slot == MWWorld::InventoryStore::Slot_CarriedRight) ? static_cast<int>(Wield::Weapon)
                                                                     : static_cast<int>(Wield::CarriedLeft);

        mParts[i] = MWAnim::attachNode(node, animation->bones, bonename);
        if (mParts[i].empty())
            return;

        Anim::Context ctx{ mParts[i], &animation->bones, &mwctx.mask };
        Anim::Update* dst = &mStaticControllers;
        if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
            dst = &mWeaponControllers;

        auto offset = autoPlay.maxPhaseGroup();
        result.link(ctx, [this, &dst, offset](const Anim::Controller* ctrl, vsg::Object* o) {
            if (ctrl->hints.autoPlay)
                autoPlay.add(ctrl, o, offset);
            else
                dst->add(ctrl, o);
        });
    }

    void WieldingCreature::addAmmo(const MWWorld::Ptr& ptr, const MWAnim::Context& mwctx)
    {
        if (!mAttachAmmo)
            return;

        const MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
        auto ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
        if (ammo != inv.end())
        {
            std::string model = ammo->getClass().getModel(*ammo);
            auto node = mwctx.readNode(model);
            auto result = MWAnim::cloneIfRequired(node);
            addSwitch(node, Wield::Ammo);
            mAttachAmmo->subgraphRequiresLocalFrustum = true;
            mAttachAmmo->addChild(node);
            mParts[static_cast<int>(Wield::Ammo)] = { mAttachAmmo, node };
        }
        // updateQuiver();
    }

    void WieldingCreature::updateEquipment()
    {
        removeParts();
        addParts();
        if (!mContext.compileContext->compile(node()))
            removeParts();
    }

}
