#ifndef VSGOPENMW_ANIMATION_CULLPARTICLES_H
#define VSGOPENMW_ANIMATION_CULLPARTICLES_H

#include <vsg/nodes/Switch.h>

#include "channel.hpp"
#include "context.hpp"
#include "mask.hpp"
#include "tmutable.hpp"
#include "deltatime.hpp"

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
        int dispatchIndex = 0;
        int drawIndex = 0;
        void apply(vsg::Switch& group, float time)
        {
            float dt = mDt.get(time);
            if (!active->value(time))
            {
                mCountdown -= dt;
                setAllChildren(mCountdown > 0.f ? mMask : vsg::MASK_OFF, group);
            }
            else
            {
                mCountdown = maxLifetime;
                group.children[drawIndex].mask = mMask;
                group.children[dispatchIndex].mask = (dt > 0) ? mMask : vsg::MASK_OFF;
            }
        }
        void setAllChildren(vsg::Mask m, vsg::Switch& sw)
        {
            for (auto& c : sw.children)
                c.mask = m;
        }
        void link(Anim::Context& ctx, vsg::Object& o)
        {
            mCountdown = 0.f;
            if (ctx.mask)
                mMask = ctx.mask->particle;
            setAllChildren(mMask, static_cast<vsg::Switch&>(o));
        }

    private:
        // float mCountdown = maxLifetime; // startEnabled
        // openmw-3677-particle-system-active
        float mCountdown = 0.f; // startDisabled
        vsg::Mask mMask = vsg::MASK_OFF;
        DeltaTime mDt;
    };
}

#endif
