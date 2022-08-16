#include "wieldingcreature.hpp"

#include <iostream>

#include <components/vsgutil/removechild.hpp>
#include <components/vsgutil/addchildren.hpp>
#include <components/mwanimation/bones.hpp>
#include <components/mwanimation/decorate.hpp>
#include <components/mwanimation/attach.hpp>
#include <components/mwanimation/play.hpp>
#include <components/mwanimation/clone.hpp>
#include <components/mwanimation/context.hpp>
#include <components/animation/context.hpp>
#include <components/animation/controllermap.hpp>

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "env.hpp"

namespace MWRender
{

WieldingCreature::WieldingCreature(const MWAnim::Context &mwctx, const MWWorld::Ptr &ptr, bool biped, const std::string &model)
    : Wielding(mwctx)
    , mPtr(ptr)
{
    auto node = mwctx.readNode(model);
    auto result = MWAnim::cloneIfRequired(node, "tri bip");

    animation = std::make_unique<MWAnim::Play>();
    animation->bones = MWAnim::Bones(*node);
    assignBones();

    std::vector<std::string> files;
    if (biped)
        files.emplace_back("x" + mwctx.files.baseanim);
    files.emplace_back(model);
    animation->addAnimSources(mwctx.readAnimations(files));

    Anim::Context ctx{node.get(), &animation->bones, &mwctx.mask};
    result.link(ctx);
    result.accept([this](const Anim::Controller *ctrl, vsg::Object *o) {
        if (ctrl->hints.autoPlay)
            autoPlay.controllers.emplace_back(ctrl, o);
        else
            mUpdate.controllers.emplace_back(ctrl, o);
        });
    vsgUtil::addChildren(*mTransform, *node);

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

    for (int i=0; i<3; ++i)
    {
        auto &path = mParts[i];
        vsgUtil::prune<vsg::Group>(path);
        path.clear();
    }
}

void WieldingCreature::update(float dt)
{
    Wielding::update(dt);
    mStaticControllers.update(0);
    autoPlay.update(dt);
    mUpdate.update(animation->time());
}

void WieldingCreature::addPart(const MWWorld::Ptr &ptr, int slot, const MWAnim::Context &mwctx)
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
        if(item.getType() == ESM::Weapon::sRecordId)
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
        //if (item.getType() == ESM::Armor::sRecordId)
            //itemModel = getShieldMesh(item, false);
    }

    auto node = mwctx.readNode(model, false);
    if (!node)
    {
        std::cerr << "!readNode(\""<<  model<< "\""<<std::endl;
        return;
    }

    auto result = MWAnim::decorate(node, {});
    addEnv(node, item);
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
    {
        mAttachAmmo = result.placeholders.attachAmmo;
        addSwitch(node, Wield::Weapon);
    }
    else
        addSwitch(node, Wield::CarriedLeft);

    int i = (slot == MWWorld::InventoryStore::Slot_CarriedRight) ? static_cast<int>(Wield::Weapon) : static_cast<int>(Wield::CarriedLeft);
    mParts[i] = MWAnim::attach(node, result.contents, *mTransform, animation->bones, bonename);
    if (mParts[i].empty())
        return;

    Anim::Context ctx{node, &animation->bones, &mwctx.mask};
    result.link(ctx);
    auto &dst = mStaticControllers;
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
        dst = mWeaponControllers;

    result.accept([this, &dst](const Anim::Controller *ctrl, vsg::Object *o) {
        if (ctrl->hints.autoPlay)
            autoPlay.controllers.emplace_back(ctrl, o);
        else
            dst.controllers.emplace_back(ctrl, o);
    });
}

void WieldingCreature::addAmmo(const MWWorld::Ptr &ptr, const MWAnim::Context &mwctx)
{
    if (!mAttachAmmo)
        return;

    const MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
    auto ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
    if (ammo != inv.end())
    {
        std::string model = ammo->getClass().getModel(*ammo);
        auto node = mwctx.readNode(model, false);
        if (!node)
        {
            std::cerr << "!readNode(\""<<  model<< "\""<<std::endl;
            return;
        }
        auto result = MWAnim::cloneIfRequired(node);
        addSwitch(node, Wield::Ammo);
        mAttachAmmo->addChild(node);
        mParts[static_cast<int>(Wield::Ammo)] = {mAttachAmmo, node};
    }
    //updateQuiver();
}

void WieldingCreature::updateEquipment()
{
    removeParts();
    addParts();
    if (!mContext.compile(node()))
        removeParts();
}

}

