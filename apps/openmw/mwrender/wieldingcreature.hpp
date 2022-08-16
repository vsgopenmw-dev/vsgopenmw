#ifndef VSGOPENMW_MWRENDER_WIELDINGCREATURE_H
#define VSGOPENMW_MWRENDER_WIELDINGCREATURE_H

#include <components/mwanimation/wielding.hpp>

#include "../mwworld/ptr.hpp"

namespace MWRender
{
    class WieldingCreature : public MWAnim::Wielding
    {
        Anim::Update mStaticControllers;
        Anim::Update mUpdate;
        void addPart(const MWWorld::Ptr &ptr, int slot, const MWAnim::Context &mwctx);
        void addAmmo(const MWWorld::Ptr &ptr, const MWAnim::Context &mwctx);
        void addParts();
        void removeParts();
        std::vector<vsg::Node*> mParts[3];
        MWWorld::Ptr mPtr;
    public:
        WieldingCreature(const MWAnim::Context &mwctx, const MWWorld::Ptr &ptr, bool biped, const std::string& model);
        void update(float dt) override;
        void updateEquipment() override;
    };
}

#endif
