#include "npc.hpp"

#include <iostream>

#include <components/vsgutil/readnode.hpp>
#include <components/vsgutil/addchildren.hpp>
#include <components/animation/context.hpp>
#include <components/animation/controllermap.hpp>
#include <components/vsgutil/removechild.hpp>
#include <components/mwanimation/face.hpp>
#include <components/mwanimation/clone.hpp>
#include <components/mwanimation/groups.hpp>
#include <components/mwanimation/bones.hpp>
#include <components/mwanimation/decorate.hpp>
#include <components/mwanimation/attach.hpp>
#include <components/mwanimation/partbones.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/weapontype.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "env.hpp"
#include "bodyparts.hpp"

namespace MWRender
{

Npc::Npc(const MWAnim::Context &mwctx, const MWWorld::Ptr& ptr, ViewMode viewMode)
    : Wielding(mwctx)
    , mPtr(ptr)
    , mViewMode(viewMode)
    , mNpcType(getNpcType(ptr))
{
    mNpc = mPtr.get<ESM::NPC>()->mBase;

    mFace = std::make_unique<MWAnim::Face>();

    for(size_t i = 0;i < ESM::PRT_Count;i++)
    {
        mPartslots[i] = -1;  //each slot is empty
        mPartPriorities[i] = 0;
    }

    updateNpcBase();
}

Npc::~Npc()
{
}

Npc::NpcType Npc::getNpcType() const
{
    const MWWorld::Class &cls = mPtr.getClass();
    // Dead vampires should typically stay vampires.
    if (mNpcType == NpcType::Vampire && cls.getNpcStats(mPtr).isDead() && !cls.getNpcStats(mPtr).isWerewolf())
        return mNpcType;
    return getNpcType(mPtr);
}

Npc::NpcType Npc::getNpcType(const MWWorld::Ptr& ptr)
{
    const MWWorld::Class &cls = ptr.getClass();
    Npc::NpcType curType = NpcType::Normal;
    if (cls.getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Vampirism).getMagnitude() > 0)
        curType = NpcType::Vampire;
    if (cls.getNpcStats(ptr).isWerewolf())
        curType = NpcType::Werewolf;
    return curType;
}

void Npc::compile()
{
    if (!mContext.compile(node()))
    {
        clearControllers();
        for (size_t i=0; i<ESM::PRT_Count; ++i)
            removeIndividualPart(static_cast<ESM::PartReferenceType>(i));
    }
}

void Npc::rebuild()
{
    //mScabbard.reset();
    //mHolsteredShield.reset();
    updateNpcBase();

    compile();

    //vsgopenmw-fixme(dependency-policy)
    MWBase::Environment::get().getMechanicsManager()->forceStateUpdate(mPtr);
}

int Npc::getSlot(const std::vector<const vsg::Node*> &path) const
{
    for (int i=0; i<ESM::PRT_Count; ++i)
    {
        const auto &comparePath = mParts[i];
        if (comparePath.empty())
            continue;
        auto i1 = comparePath.begin();
        auto i2 = path.begin();
        while (i2 != path.end() && i1 != comparePath.end())
        {
            if (*i1 == *i2)
                ++i1;
            ++i2;
        }
        if (i1 == comparePath.end())
            return mPartslots[i];
    }
    return -1;
}

void Npc::updateNpcBase()
{
    for(size_t i = 0;i < ESM::PRT_Count;i++)
        removeIndividualPart((ESM::PartReferenceType)i);

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const ESM::Race *race = store.get<ESM::Race>().find(mNpc->mRace);
    NpcType curType = getNpcType();
    bool isWerewolf = (curType == NpcType::Werewolf);
    bool isVampire = (curType == NpcType::Vampire);
    bool isFemale = !mNpc->isMale();

    auto &bodyPartStore = store.get<ESM::BodyPart>();
    mHeadModel = getHeadModel(isWerewolf, isVampire, isFemale, mNpc->mRace, mNpc->mHead, bodyPartStore);
    mHairModel = getHairModel(isWerewolf, mNpc->mHair, bodyPartStore);

    bool is1stPerson = mViewMode == ViewMode::FirstPerson;
    bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;

    std::string defaultSkeleton = mContext.pickSkeleton(is1stPerson, isFemale, isBeast, isWerewolf);
    defaultSkeleton = mContext.findActorModelCallback(defaultSkeleton);

    std::string smodel = defaultSkeleton;
    if (!is1stPerson && !isWerewolf && !mNpc->mModel.empty())
        smodel = mContext.findActorModelCallback(mNpc->mModel);

    std::vector<std::string> sources;
    if(!is1stPerson)
    {
        const std::string base = "x" + mContext.files.baseanim;
        if (smodel != base && !isWerewolf)
            sources.emplace_back(base);

        if (smodel != defaultSkeleton && base != defaultSkeleton)
            sources.emplace_back(defaultSkeleton);

        sources.emplace_back(smodel);

        if(!isWerewolf && Misc::StringUtils::lowerCase(mNpc->mRace).find("argonian") != std::string::npos)
            sources.emplace_back("xargonian_swimkna.nif");
    }
    else
    {
        const std::string base = "x" + mContext.firstPersonFiles.baseanim;
        if (smodel != base && !isWerewolf)
            sources.emplace_back(base);
        sources.emplace_back(smodel);
    }

    vsg::ref_ptr<const vsg::Node> node = mContext.readNode(smodel);

    animation = std::make_unique<MWAnim::Play>(MWAnim::sNumBlendMasks);
    animation->bones = MWAnim::Bones(static_cast<const vsg::Node&>(*node), MWAnim::groups);
    animation->addAnimSources(mContext.readAnimations(sources));
    assignBones();
    updateParts();
}

/*
std::string Npc::getSheathedShieldMesh(const MWWorld::ConstPtr& shield) const
{
    std::string mesh = getShieldMesh(shield, !mNpc->isMale());

    if (mesh.empty())
        return std::string();

    std::string holsteredName = mesh;
    holsteredName = holsteredName.replace(holsteredName.size()-4, 4, "_sh.nif");
    if(mResourceSystem->getVFS()->exists(holsteredName))
    {
        osg::ref_ptr<osg::Node> shieldTemplate = mResourceSystem->getSceneManager()->getInstance(holsteredName);
        SceneUtil::FindByNameVisitor findVisitor ("Bip01 Sheath");
        shieldTemplate->accept(findVisitor);
        osg::ref_ptr<osg::Node> sheathNode = findVisitor.mFoundNode;
        if(!sheathNode)
            return std::string();
    }

    return mesh;
}
*/

void Npc::updateEquipment()
{
/*    static const bool shieldSheathing = Settings::Manager::getBool("shield sheathing", "Game");
    if (shieldSheathing)
    {
        int weaptype = ESM::Weapon::None;
        MWMechanics::getActiveWeapon(mPtr, &weaptype);
        showCarriedLeft(updateCarriedLeftVisible(weaptype));
    }
*/
    updateParts();
    compile();
}

void Npc::clearControllers()
{
    autoPlay.controllers.clear();
    mStaticControllers.controllers.clear();
    mFace->controllers.clear();
    mWeaponControllers.controllers.clear();
}

void Npc::updateParts()
{
    NpcType curType = getNpcType();
    if (curType != mNpcType)
    {
        mNpcType = curType;
        rebuild();
        return;
    }

    clearControllers();
    soundsToPlay.clear();

    static const struct {
        int slot;
        int basePriority;
    } slotlist[] = {
        // FIXME: Priority is based on the number of reserved slots. There should be a better way.
        { MWWorld::InventoryStore::Slot_Robe,         11 },
        { MWWorld::InventoryStore::Slot_Skirt,         3 },
        { MWWorld::InventoryStore::Slot_Helmet,        0 },
        { MWWorld::InventoryStore::Slot_Cuirass,       0 },
        { MWWorld::InventoryStore::Slot_Greaves,       0 },
        { MWWorld::InventoryStore::Slot_LeftPauldron,  0 },
        { MWWorld::InventoryStore::Slot_RightPauldron, 0 },
        { MWWorld::InventoryStore::Slot_Boots,         0 },
        { MWWorld::InventoryStore::Slot_LeftGauntlet,  0 },
        { MWWorld::InventoryStore::Slot_RightGauntlet, 0 },
        { MWWorld::InventoryStore::Slot_Shirt,         0 },
        { MWWorld::InventoryStore::Slot_Pants,         0 },
        { MWWorld::InventoryStore::Slot_CarriedLeft,   0 },
        { MWWorld::InventoryStore::Slot_CarriedRight,  0 }
    };
    static const size_t slotlistsize = sizeof(slotlist)/sizeof(slotlist[0]);

    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    for(size_t i = 0;i < slotlistsize && mViewMode != ViewMode::HeadOnly;i++)
    {
        auto store = inv.getSlot(slotlist[i].slot);

        removePartGroup(slotlist[i].slot);

        if(store == inv.end())
            continue;

        if(slotlist[i].slot == MWWorld::InventoryStore::Slot_Helmet)
            removeIndividualPart(ESM::PRT_Hair);

        int prio = 1;
        auto glowColor = getGlowColor(*store);
        if(store->getType() == ESM::Clothing::sRecordId)
        {
            prio = ((slotlist[i].basePriority+1)<<1) + 0;
            const ESM::Clothing *clothes = store->get<ESM::Clothing>()->mBase;
            addPartGroup(slotlist[i].slot, prio, clothes->mParts.mParts, glowColor);
        }
        else if(store->getType() == ESM::Armor::sRecordId)
        {
            prio = ((slotlist[i].basePriority+1)<<1) + 1;
            const ESM::Armor *armor = store->get<ESM::Armor>()->mBase;
            addPartGroup(slotlist[i].slot, prio, armor->mParts.mParts, glowColor);
        }

        if(slotlist[i].slot == MWWorld::InventoryStore::Slot_Robe)
        {
            ESM::PartReferenceType parts[] = {
                ESM::PRT_Groin, ESM::PRT_Skirt, ESM::PRT_RLeg, ESM::PRT_LLeg,
                ESM::PRT_RUpperarm, ESM::PRT_LUpperarm, ESM::PRT_RKnee, ESM::PRT_LKnee,
                ESM::PRT_RForearm, ESM::PRT_LForearm, ESM::PRT_Cuirass
            };
            size_t parts_size = sizeof(parts)/sizeof(parts[0]);
            for(size_t p = 0;p < parts_size;++p)
                reserveIndividualPart(parts[p], slotlist[i].slot, prio);
        }
        else if(slotlist[i].slot == MWWorld::InventoryStore::Slot_Skirt)
        {
            reserveIndividualPart(ESM::PRT_Groin, slotlist[i].slot, prio);
            reserveIndividualPart(ESM::PRT_RLeg, slotlist[i].slot, prio);
            reserveIndividualPart(ESM::PRT_LLeg, slotlist[i].slot, prio);
        }
    }

    if(mViewMode != ViewMode::FirstPerson)
    {
        if(mPartPriorities[ESM::PRT_Head] < 1 && !mHeadModel.empty())
            addOrReplaceIndividualPart(ESM::PRT_Head, -1,1, mHeadModel);
        if(mPartPriorities[ESM::PRT_Hair] < 1 && mPartPriorities[ESM::PRT_Head] <= 1 && !mHairModel.empty())
            addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1, mHairModel);
    }
    if(mViewMode == ViewMode::HeadOnly)
        return;

    /*
    if(mPartPriorities[ESM::PRT_Shield] < 1)
    {
        MWWorld::ConstContainerStoreIterator store = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        MWWorld::ConstPtr part;
        if(store != inv.end() && (part=*store).getType() == ESM::Light::sRecordId)
        {
            const ESM::Light *light = part.get<ESM::Light>()->mBase;
            addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft, 1, light->mModel, {}, light);
        }
    }
    */

    addWeapon();
    addCarriedLeft();
    addAmmo();

    bool isWerewolf = (getNpcType() == NpcType::Werewolf);
    std::string race = (isWerewolf ? "werewolf" : Misc::StringUtils::lowerCase(mNpc->mRace));

    const std::vector<const ESM::BodyPart*> &parts = getBodyParts(race, !mNpc->isMale(), mViewMode == ViewMode::FirstPerson, isWerewolf, MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>());
    for(int part = ESM::PRT_Neck; part < ESM::PRT_Count; ++part)
    {
        if(mPartPriorities[part] < 1)
        {
            const ESM::BodyPart* bodypart = parts[part];
            if(bodypart)
                addOrReplaceIndividualPart((ESM::PartReferenceType)part, -1, 1, bodypart->mModel);
        }
    }
}

void Npc::insertBoundedPart(ESM::PartReferenceType type, const std::string& model, const std::string& bonename, std::optional<vsg::vec4> glowColor, const ESM::Light *light={})
{
    auto node = vsgUtil::readNode(model, mContext.partBoneOptions[type], false);
    if (!node)
    {
        std::cerr << "!readNode(\""<<  model<< "\""<<std::endl;
        return;
    }

    bool mirror = bonename.find("Left") != std::string::npos;
    auto result = MWAnim::decorate(node, light, mirror);
    addEnv(node, glowColor);
    if (type == ESM::PRT_Weapon)
    {
        mAttachAmmo = result.placeholders.attachAmmo;
        addSwitch(node, Wield::Weapon);
    }
    else if (type == ESM::PRT_Shield)
        addSwitch(node, Wield::CarriedLeft);

    auto path = MWAnim::attach(node, result.contents, *mTransform, animation->bones, bonename);
    if (path.empty())
        return;

    Anim::Context ctx{node, &animation->bones, &mContext.mask};
    result.link(ctx);
    auto *dst = &mStaticControllers.controllers;
    if (type == ESM::PRT_Head)
    {
        mFace->loadTags(*node);
        dst = &mFace->controllers;
    }
    else if (type == ESM::PRT_Weapon)
        dst = &mWeaponControllers.controllers;

    result.accept([this, dst, type](const Anim::Controller *ctrl, vsg::Object *o) {
        if (ctrl->hints.autoPlay && type != ESM::PRT_Head)
            autoPlay.controllers.emplace_back(ctrl, o);
        else
            dst->emplace_back(ctrl, o);
    });
    mParts[type] = path;
}

void Npc::update(float dt)
{
    Wielding::update(dt);
    autoPlay.update(dt);
    mStaticControllers.update(0.f);
    mFace->update(dt);
}

void Npc::removeIndividualPart(ESM::PartReferenceType type)
{
    mPartPriorities[type] = 0;
    mPartslots[type] = -1;

    auto &path = mParts[type];
    vsgUtil::prune<vsg::Group>(path);
    path.clear();
}

void Npc::reserveIndividualPart(ESM::PartReferenceType type, int group, int priority)
{
    if(priority > mPartPriorities[type])
    {
        removeIndividualPart(type);
        mPartPriorities[type] = priority;
        mPartslots[type] = group;
    }
}

void Npc::removePartGroup(int group)
{
    for(int i = 0; i < ESM::PRT_Count; i++)
    {
        if(mPartslots[i] == group)
            removeIndividualPart((ESM::PartReferenceType)i);
    }
}

bool Npc::addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority, const std::string &mesh, std::optional<vsg::vec4> glowColor, const ESM::Light *light)
{
    if(priority <= mPartPriorities[type])
        return false;

    removeIndividualPart(type);
    mPartslots[type] = group;
    mPartPriorities[type] = priority;
    vsg::ref_ptr<vsg::Node> animGroup;
    std::string bonename = MWAnim::sPartBones.at(type);
    if (type == ESM::PRT_Weapon)
    {
        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        auto weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if(weapon != inv.end() && weapon->getType() == ESM::Weapon::sRecordId)
        {
            int weaponType = weapon->get<ESM::Weapon>()->mBase->mData.mType;
            const std::string weaponBonename = MWMechanics::getWeaponType(weaponType)->mAttachBone;

            if (weaponBonename != bonename)
            {
                if (auto found = animation->bones.search(weaponBonename))
                    bonename = weaponBonename;
            }
        }
    }

    insertBoundedPart(type, mesh, bonename, glowColor);

    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    auto csi = inv.getSlot(group < 0 ? MWWorld::InventoryStore::Slot_Helmet : group);
    if (csi != inv.end())
    {
        const auto soundId = csi->getClass().getSound(*csi);
        if (!soundId.empty())
            soundsToPlay.insert(soundId);
    }
    return true;
}

void Npc::addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts, std::optional<vsg::vec4> glowColor)
{
    bool firstPerson = (mViewMode == ViewMode::FirstPerson);
    bool female = !mNpc->isMale();
    auto &bodyPartStore = MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>();
    for(const ESM::PartReference& part : parts)
    {
        if (auto bodypart = getBodyPart(firstPerson, female, part, bodyPartStore))
            addOrReplaceIndividualPart((ESM::PartReferenceType)part.mPart, group, priority, bodypart->mModel,  glowColor);
        else
            reserveIndividualPart((ESM::PartReferenceType)part.mPart, group, priority);
    }
}

void Npc::addWeapon()
{
    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    auto weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
    if(weapon != inv.end())
    {
        std::string mesh = weapon->getClass().getModel(*weapon);
        addOrReplaceIndividualPart(ESM::PRT_Weapon, MWWorld::InventoryStore::Slot_CarriedRight, 1, mesh, getGlowColor(*weapon));
    }
    else
        removeIndividualPart(ESM::PRT_Weapon);
    //updateHolsteredWeapon(!mShowWeapons);
    //updateQuiver();
}

void Npc::addCarriedLeft()
{
    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    auto iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
    if(iter != inv.end())
    {
        std::string mesh = iter->getClass().getModel(*iter);
        // For shields we must try to use the body part model
        if (iter->getType() == ESM::Armor::sRecordId)
        {
            //mesh = getShieldMesh(*iter, !mNpc->isMale());
        }
        /*if (mesh.empty() || */
        const ESM::Light *light{};
        if(iter->getType() == ESM::Light::sRecordId)
            light = iter->get<ESM::Light>()->mBase;
        addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft, 1, mesh, getGlowColor(*iter), light);
        //if (mesh.empty())
            //reserveIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft, 1);
    }
    else
        removeIndividualPart(ESM::PRT_Shield);

    //updateHolsteredShield(mShowCarriedLeft);
}

void Npc::addAmmo()
{
    if (!mAttachAmmo)
        return;

    const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    auto ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
    if (ammo != inv.end())
    {
        std::string model = ammo->getClass().getModel(*ammo);
        auto node = mContext.readNode(model, false);
        if (!node)
        {
            std::cerr << "!readNode(\""<<  model<< "\""<<std::endl;
            return;
        }
        auto result = MWAnim::cloneIfRequired(node);
        addSwitch(node, Wield::Ammo);
        mAttachAmmo->addChild(node);
    }
    //updateQuiver();
}

void Npc::setFacialState(bool enable, bool talking, float loudness)
{
    mFace->enabled = enable;
    mFace->talking = talking;
    mFace->loudness = loudness;
}

void Npc::setVampire(bool vampire)
{
    if (mNpcType == NpcType::Werewolf) // we can't have werewolf vampires, can we
        return;
    if ((mNpcType == NpcType::Vampire) != vampire)
    {
        if (mPtr == MWMechanics::getPlayer())
            MWBase::Environment::get().getWorld()->reattachPlayerCamera();
        else
            rebuild();
    }
}

void Npc::updatePtr(const MWWorld::Ptr &updated)
{
    mPtr = updated;
}

}
