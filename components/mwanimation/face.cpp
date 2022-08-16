#include "face.hpp"

#include <components/animation/tags.hpp>
#include <components/animation/visitor.hpp>
#include <components/vsgutil/searchbytype.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/stringops.hpp>

namespace MWAnim
{

Face::Face()
{
    resetBlinkTimer();
}

void Face::resetBlinkTimer()
{
    mBlinkTimer = -(2.0f + Misc::Rng::rollDice(6));
}

void Face::update(float dt)
{
    if (enabled && dt > 0)
    {
        if (!talking)
        {
            mBlinkTimer += dt;
            float duration = mBlinkStop - mBlinkStart;
            if (mBlinkTimer >= 0 && mBlinkTimer <= duration)
                mValue = mBlinkStart + mBlinkTimer;
            else
                mValue = mBlinkStop;
            if (mBlinkTimer > duration)
                resetBlinkTimer();
        }
        else
            mValue = mTalkStart + (mTalkStop - mTalkStart) * std::min(1.f, loudness*2); // Rescale a bit (most voices are not very loud)
    }
    Anim::Update::update(mValue);
}

void Face::loadTags(vsg::Object &node)
{
    if (auto tags = vsgUtil::searchFirst<const Anim::Tags, Anim::Visitor>(node))
    {
        for (const auto &key : *tags)
        {
            if (Misc::StringUtils::ciEqual(key.second, "talk: start"))
                mTalkStart = key.first;
            if (Misc::StringUtils::ciEqual(key.second, "talk: stop"))
                mTalkStop = key.first;
            if (Misc::StringUtils::ciEqual(key.second, "blink: start"))
                mBlinkStart = key.first;
            if (Misc::StringUtils::ciEqual(key.second, "blink: stop"))
                mBlinkStop = key.first;
        }
    }
}

}
