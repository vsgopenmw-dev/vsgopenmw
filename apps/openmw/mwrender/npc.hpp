#ifndef VSGOPENMW_MWRENDER_NPC_H
#define VSGOPENMW_MWRENDER_NPC_H

#include <array>
#include <map>
#include <optional>
#include <set>

#include <vsg/maths/vec4.h>

#include <components/mwanimation/context.hpp>
#include <components/mwanimation/wielding.hpp>
#include <components/animation/update.hpp>
#include <components/esm3/loadarmo.hpp>

#include "../mwworld/ptr.hpp"

namespace ESM
{
    class Light;
    class NPC;
}
namespace MWAnim
{
    class Face;
    class Context;
}
namespace MWRender
{

class Npc : public MWAnim::Wielding
{
    MWWorld::Ptr mPtr;
public:
    enum class ViewMode {
        Normal,
        FirstPerson,
        HeadOnly
    };
    Npc(const MWAnim::Context &mwctx, const MWWorld::Ptr &ptr, ViewMode viewMode=ViewMode::Normal);
    ~Npc();

    MWWorld::Ptr getPtr() { return mPtr; }

    void setFacialState(bool enable, bool talking, float loudness);

    void update(float dt) override;

    void updateParts();
    void updateEquipment() override;

    /// Rebuilds the NPC, updating their root model, animation sources, and equipment.
    void rebuild();

    /// Get the inventory slot that the given node path leads into, or -1 if not found.
    int getSlot(const std::vector<const vsg::Node*> &path) const;

    void setVampire(bool vampire);

    void updatePtr(const MWWorld::Ptr& updated) /*override*/;

    std::set<std::string> soundsToPlay;

protected:
    Anim::Update mStaticControllers;

    std::vector<vsg::Node*> mParts[ESM::PRT_Count];

    const ESM::NPC *mNpc;
    std::string mHeadModel;
    std::string mHairModel;
    ViewMode mViewMode;

    enum class NpcType
    {
        Normal,
        Werewolf,
        Vampire
    } mNpcType;

    int mPartslots[ESM::PRT_Count]; //Each part slot is taken by clothing, armor, or is empty
    int mPartPriorities[ESM::PRT_Count];

    std::unique_ptr<MWAnim::Face> mFace;

    void updateNpcBase();

    NpcType getNpcType() const;

    void insertBoundedPart(ESM::PartReferenceType type, const std::string &model, const std::string &bonename, std::optional<vsg::vec4> glowColor, const ESM::Light *light);

    void removeIndividualPart(ESM::PartReferenceType type);
    void reserveIndividualPart(ESM::PartReferenceType type, int group, int priority);

    bool addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority, const std::string &mesh, std::optional<vsg::vec4> glowColor={}, const ESM::Light *light = {});
    void removePartGroup(int group);
    void addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts, std::optional<vsg::vec4> glowColor={});
    void addWeapon();
    void addCarriedLeft();
    void addAmmo();

    static NpcType getNpcType(const MWWorld::Ptr& ptr);
    void clearControllers();
    void compile();
};

}

#endif
