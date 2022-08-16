#ifndef VSGOPENMW_MWRENDER_PLAYER_H
#define VSGOPENMW_MWRENDER_PLAYER_H

#include "npc.hpp"

namespace MWRender
{
    /*
     * Supports first person view.
     */
    class Player : public Npc
    {
        Anim::Bone *mNeck{};
        float mAimingFactor = 1.f;
        void assignBones();
    public:
        Player(const MWAnim::Context &mwctx, const MWWorld::Ptr& ptr, ViewMode viewMode=ViewMode::Normal, float firstPersonFieldOfView=55.f);
        ~Player();
        void update(float dt) override;
        void setViewMode(Npc::ViewMode m);

        vsg::vec3 firstPersonOffset;
        bool accurateAiming{};
        // Externally set true when important animations are currently playing on the upper body.
        bool busy = false;
    };
}

#endif
