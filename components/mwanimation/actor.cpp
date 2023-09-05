#include "actor.hpp"

#include <vsg/maths/transform.h>

#include <components/animation/bones.hpp>
#include <components/animation/rotate.hpp>

#include "play.hpp"

namespace MWAnim
{
    Actor::Actor(const Context& ctx)
        : ObjectAnimation(ctx)
    {
    }

    Actor::~Actor() {}

    void Actor::manualAnimation(float pitchSpine, float dt)
    {
        const float epsilon = 0.001f;
        float yawOffset = 0;
        if (mRoot)
        {
            bool enable = std::abs(legsYawRadians) > epsilon || std::abs(bodyPitchRadians) > epsilon;
            if (enable && animation->isRotationControlled(*mRoot))
            {
                Anim::rotateBone(*mRoot, vsg::rotate(bodyPitchRadians, { 1, 0, 0 }) * vsg::rotate(legsYawRadians, { 0, 0, 1 }));
                yawOffset = legsYawRadians;
            }
        }
        if (mSpine[0])
        {
            float yaw = upperBodyYawRadians - yawOffset;
            bool enable = std::abs(yaw) > epsilon;
            if (enable && animation->isRotationControlled(*mSpine[0]))
            {
                Anim::rotateBone(*mSpine[0], vsg::rotate(yaw, { 0, 0, 1 }));
                yawOffset = upperBodyYawRadians;
            }
        }
        if (mHead)
        {
            float yaw = headYawRadians - yawOffset;
            bool enable = (std::abs(headPitchRadians) > epsilon || std::abs(yaw) > epsilon);
            if (enable && animation->isRotationControlled(*mHead))
                Anim::rotateBone(*mHead, vsg::rotate(yaw, { 0, 0, 1 }) * vsg::rotate(headPitchRadians, { 1, 0, 0 }));
        }

        if (spinePitchFactor == 0)
            return;
        pitchSpine += bodyPitchRadians;
        for (auto& spine : mSpine)
        {
            if (!spine || !animation->isRotationControlled(*spine))
                continue;
            Anim::rotateBone(*spine, vsg::rotate(pitchSpine * spinePitchFactor / 2, { -1, 0, 0 }));
        }
    }

    void Actor::assignBones()
    {
        auto& bones = animation->bones;
        mHead = bones.search("bip01 head");
        mSpine[0] = bones.search("bip01 spine1");
        mSpine[1] = bones.search("bip01 spine2");
        mRoot = bones.search("bip01");
    }
}
