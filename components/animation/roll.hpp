#ifndef VSGOPENMW_ANIMATION_ROLL_H
#define VSGOPENMW_ANIMATION_ROLL_H

#include "channel.hpp"
#include "deltatime.hpp"
#include "tmutable.hpp"
#include "transform.hpp"

namespace Anim
{
    /*
     * Rotates around axis based on speed and delta time.
     */
    class Roll : public TMutable<Roll, Transform>
    {
        DeltaTime mDt;
    public:
        channel_ptr<float> speed;
        vsg::vec3 axis{ 0, 0, 1 };
        void apply(Transform& transform, float time);
    };
}

#endif
