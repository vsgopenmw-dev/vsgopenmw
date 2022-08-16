#include "player.hpp"

#include <vsg/maths/transform.h>

#include <components/mwanimation/rotate.hpp>
#include <components/mwanimation/play.hpp>

namespace MWRender
{
    Player::Player(const MWAnim::Context &mwctx, const MWWorld::Ptr& ptr, ViewMode viewMode, float firstPersonFieldOfView)
        : Npc(mwctx, ptr, viewMode)
    {
        assignBones();
    }

    Player::~Player()
    {
    }

    void Player::update(float dt)
    {
        if (mNeck && animation->isRotationControlled(*mNeck))
        {
            if (accurateAiming)
                mAimingFactor = 1.f;
            else
                mAimingFactor = std::max(0.f, mAimingFactor - dt * 0.5f);
            float rotateFactor = 0.75f + 0.25f * mAimingFactor;
            MWAnim::rotate(*mNeck, vsg::rotate(getPtr().getRefData().getPosition().rot[0] * rotateFactor, vsg::vec3(-1,0,0)), firstPersonOffset);
        }
        Npc::update(dt);
    }

    void Player::setViewMode(Npc::ViewMode m)
    {
        if(mViewMode == m)
            return;
        mViewMode = m;
        //mAmmunition.reset();
        rebuild();
        assignBones();
    }

    void Player::assignBones()
    {
        if (mViewMode == ViewMode::FirstPerson)
            mNeck = animation->bones.search("bip01 neck");
        else
            mNeck = {};
        Npc::assignBones();
    }
}
