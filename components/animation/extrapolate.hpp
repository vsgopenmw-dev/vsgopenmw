#ifndef VSGOPENMW_ANIMATION_EXTRAPOLATE_H
#define VSGOPENMW_ANIMATION_EXTRAPOLATE_H

#include <cmath>
#include <cassert>

#include "channel.hpp"

namespace Anim
{
    enum class ExtrapolationMode
    {
        Cycle,
        Reverse,
        Constant
    };

    struct ExtrapolationSettings
    {
        float frequency = 1.f;
        float phase = 0.f;
        float startTime = 0.f;
        float stopTime = 1.f;
    };

    /*
     * Adapts time.
     */
    template<class T, ExtrapolationMode M>
    class Extrapolate : public Channel<T>
    {
    public:
        Extrapolate(channel_ptr<T> c, ExtrapolationSettings &s) : channel(c), settings(s), delta(settings.stopTime - settings.startTime)
        {
            assert(delta > 0);
        }
        const channel_ptr<T> channel;
        const ExtrapolationSettings settings;
        const float delta;
        T value(float time) const override
        {
            return channel->value(extrapolate(time));
        }
        float extrapolate (float time) const
        {
            time = settings.frequency * time + settings.phase;
            switch (M)
            {
            case ExtrapolationMode::Cycle:
            {
                float cycles = (time - settings.startTime) / delta;
                float remainder = (cycles - std::floor(cycles)) * delta;
                return settings.startTime + remainder;
            }
            case ExtrapolationMode::Reverse:
            {
                float cycles = (time - settings.startTime) / delta;
                float remainder = (cycles - std::floor(cycles)) * delta;

                if ((static_cast<int>(std::fabs(std::floor( cycles))) % 2) == 0)
                    return settings.startTime + remainder;

                return settings.stopTime - remainder;
            }
            case ExtrapolationMode::Constant:
                return std::clamp(time, settings.startTime, settings.stopTime);
            }
        }
    };
}

#endif
