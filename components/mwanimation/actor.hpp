#ifndef VSGOPENMW_MWANIMATION_ACTOR_H
#define VSGOPENMW_MWANIMATION_ACTOR_H

#include <array>

#include "objectanim.hpp"

namespace MWAnim
{
    /*
     * Manually animates bones.
     */
    class Actor : public ObjectAnimation
    {
    public:
        Actor(const Context& ctx);
        ~Actor();
        float headYawRadians = 0;
        float headPitchRadians = 0;
        float upperBodyYawRadians = 0;
        float legsYawRadians = 0;
        float bodyPitchRadians = 0;

        /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
        /// to indicate the facing orientation of the character, for ranged weapon aiming.
        float spinePitchFactor = 0;

        // void manualAnimation(float headYaw...
        virtual void manualAnimation(float pitchSpine, float dt);
        const Anim::Bone* getHead() { return mHead; }

        void assignBones();

    protected:
        Anim::Bone* mHead{};
        std::array<Anim::Bone*, 2> mSpine;
        Anim::Bone* mRoot{};
    };
}

#endif
