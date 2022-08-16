#ifndef VSGOPENMW_ANIMATION_RANGE_H
#define VSGOPENMW_ANIMATION_RANGE_H

#include "channel.hpp"

namespace Anim
{
    /*
     * Activates in the specified time range.
     */
    class Range : public Anim::Channel<bool>
    {
    public:
        Range(float start, float stop) : startTime(start), stopTime(stop) {}
        float startTime = 0.f;
        float stopTime = 0.f;
        bool value(float time) const override
        {
            return time >= startTime && time < stopTime;
        }
    };
}

#endif
