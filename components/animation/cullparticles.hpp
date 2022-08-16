#ifndef VSGOPENMW_ANIMATION_CULLPARTICLES_H
#define VSGOPENMW_ANIMATION_CULLPARTICLES_H

#include <vsg/nodes/Switch.h>

#include "tmutable.hpp"
#include "channel.hpp"
#include "context.hpp"
#include "mask.hpp"

namespace Anim
{
    /*
     * Switches off node once it's been inactive for the specified time period.
     */
    class CullParticles : public Anim::TMutable<CullParticles, vsg::Switch>
    {
    public:
        Anim::channel_ptr<bool> active;
        float maxLifetime = 0.f;
        void apply(vsg::Switch &group, float time)
        {
            float dt = time - mLastTime;
            mLastTime = time;
            if (!active->value(time))
            {
                mCountdown -= dt;
                setAllChildren(mCountdown > 0.f ? mMask : vsg::MASK_OFF, group);
            }
            else
            {
                mCountdown = maxLifetime;
                setAllChildren(mMask, group);
            }
        }
        void setAllChildren(vsg::Mask m, vsg::Switch &sw)
        {
            for (auto &c : sw.children)
                c.mask = m;
        }
        void link(Anim::Context &ctx, vsg::Object &o)
        {
            mLastTime = 0.f;
            mCountdown = 0.f;
            mMask = ctx.mask->particle;
            setAllChildren(mMask, static_cast<vsg::Switch&>(o));
        }
    private:
        float mLastTime = 0.f;
        float mCountdown = 0.f;
        vsg::Mask mMask = vsg::MASK_OFF;
    };
}

#endif
