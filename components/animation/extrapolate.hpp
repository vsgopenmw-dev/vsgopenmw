#ifndef VSGOPENMW_ANIMATION_EXTRAPOLATE_H
#define VSGOPENMW_ANIMATION_EXTRAPOLATE_H

#include <cassert>
#include <cmath>
#include <algorithm>

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
     * Extrapolate<T> is a composite channel class used to adapt the time value that is passed in.
     * The extrapolation mode to use is passed as a template parameter to avoid the evaluation of branches at runtime.
     */
    template <class T, ExtrapolationMode M, bool TimeFunc>
    class Extrapolate : public Channel<T>
    {
    public:
        Extrapolate(channel_ptr<T> c, const ExtrapolationSettings& s)
            : channel(c)
            , settings(s)
            , delta(settings.stopTime - settings.startTime)
        {
            assert(delta > 0);
        }
        const channel_ptr<T> channel;
        const ExtrapolationSettings settings;
        const float delta;
        T value(float time) const override { return channel->value(extrapolate(time)); }
        float extrapolate(float time) const
        {
            if (TimeFunc)
                time = settings.frequency * time + settings.phase;
            switch (M)
            {
                case ExtrapolationMode::Cycle:
                {
                    if (TimeFunc)
                        time -= settings.startTime;
                    float cycles = time / delta;
                    float remainder = (cycles - std::floor(cycles)) * delta;
                    return TimeFunc ? settings.startTime + remainder : remainder;
                }
                case ExtrapolationMode::Reverse:
                {
                    if (TimeFunc)
                        time -= settings.startTime;
                    float cycles = time / delta;
                    float remainder = (cycles - std::floor(cycles)) * delta;

                    if ((static_cast<int>(std::fabs(std::floor(cycles))) % 2) == 0)
                        return TimeFunc ? settings.startTime + remainder : remainder;

                    return settings.stopTime - remainder;
                }
                case ExtrapolationMode::Constant:
                    return std::clamp(time, settings.startTime, settings.stopTime);
            }
        }
    };

    template <class T, ExtrapolationMode M>
    channel_ptr<T> makeExtrapolator(const channel_ptr<T>& channel, const ExtrapolationSettings& settings)
    {
        if (settings.frequency == 1 && settings.phase == 0 && settings.startTime == 0)
            return vsg::ref_ptr{ new Extrapolate<T, M, false>(channel, settings) };
        else
            return vsg::ref_ptr{ new Extrapolate<T, M, true>(channel, settings) };
    }

    template <class T>
    channel_ptr<T> makeExtrapolator(channel_ptr<T> channel, const ExtrapolationSettings& settings, ExtrapolationMode mode)
    {
        if (mode == ExtrapolationMode::Constant)
            return makeExtrapolator<T, ExtrapolationMode::Constant>(channel, settings);
        else if (mode == ExtrapolationMode::Reverse)
            return makeExtrapolator<T, ExtrapolationMode::Reverse>(channel, settings);
        else if (mode == ExtrapolationMode::Cycle)
            return makeExtrapolator<T, ExtrapolationMode::Cycle>(channel, settings);
    }
}

#endif
