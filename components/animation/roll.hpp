#ifndef VSGOPENMW_ANIMATION_ROLL_H
#define VSGOPENMW_ANIMATION_ROLL_H

#include "tmutable.hpp"
#include "channel.hpp"
#include "transform.hpp"

namespace Anim
{
    class Roll : public TMutable<Roll, Transform>
    {
    public:
        channel_ptr<float> speed;
        vsg::vec3 axis{0,0,1};

        void apply(Transform &transform, float time);
        void link(Context &, vsg::Object &) override { mLastTime = /*context.time*/0.f; }
    private:
        float mLastTime = 0.f;
    };
}

#endif
