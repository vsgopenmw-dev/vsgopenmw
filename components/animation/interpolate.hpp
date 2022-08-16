#ifndef VSGOPENMW_ANIMATION_INTERPOLATE_H
#define VSGOPENMW_ANIMATION_INTERPOLATE_H

#include <map>

#include <vsg/maths/quat.h>
#include <vsg/maths/transform.h>

#include "channel.hpp"

namespace Anim
{
    /*
     * Provides interpolation function interface.
     */
    template <typename T>
    struct Interpolate
    {
        using value_type = T;
        inline T access(const T &t) const
        {
            return t;
        }
        inline T operator()(const T &a, const T &b, float fraction) const = delete;
    };

    /*
     * Applies interpolation function to a keyframe container.
     */
    template <typename Map, typename Func>
    inline typename Func::value_type t_interpolate(const Map &keys, const Func &interpolator, float time)
    {
        //vsgopenmw-optimize-me(stateless-keyframe-lookup)
    //using Map = std::unordered_multimap<float, T, Equal(quantize(key, maxKeyframeDistance))>;
    //auto range = map.equal_range(t);

        if(time <= keys.begin()->first)
            return interpolator.access(keys.begin()->second);
        auto it = keys.lower_bound(time);
        if (it != keys.end())
        {
            auto prev = it;
            --prev;
            float fraction = (time - prev->first) / (it->first - prev->first);
            return interpolator(prev->second, it->second, fraction);
        }
        else
            return interpolator.access(keys.rbegin()->second);
    }

    /*
     * Implements interpolation functions.
     */
    template <typename T>
    class Flip : public Interpolate<T>, public Channel<T>
    {
    public:
        std::map<float, T> keys;
        T value(float time) const override
        {
            return t_interpolate(keys, *this, time);
        }
        inline T operator()(const T &a, const T &b, float fraction) const
        {
            return fraction > 0.5f ? b : a;
        }
    };
    template <typename T>
    class Linear : public Interpolate<T>, public Channel<T>
    {
    public:
        std::map<float, T> keys;
        T value(float time) const override
        {
            return t_interpolate(keys, *this, time);
        }
        inline T operator()(const T &a, const T &b, float fraction) const
        {
            return vsg::mix(a, b, fraction);
        }
    };
    template <typename T>
    class CubicHermiteSpline : public Interpolate<T>, public Channel<T>
    {
    public:
        struct Data
        {
            T value;
            T inTan;
            T outTan;
        };
        std::map<float, Data> keys;
        T value(float time) const override
        {
            return t_interpolate(keys, *this, time);
        }
        inline T access(const Data &data) const
        {
            return data.value;
        }
        inline T operator()(const Data &a, const Data &b, float t) const
        {
            auto t2 = t * t;
            auto t3 = t2 * t;
            auto b1 = 2.f * t3 - 3.f * t2 + 1;
            auto b2 = -2.f * t3 + 3.f * t2;
            auto b3 = t3 - 2.f * t2 + t;
            auto b4 = t3 - t2;
            return a.value * b1 + b.value * b2 + a.outTan * b3 + b.inTan * b4;
        }
    };
    template <>
    class CubicHermiteSpline <vsg::quat> : public Interpolate<vsg::quat>, public Channel<vsg::quat>
    {
    public:
        std::map<float, vsg::quat> keys;
        vsg::quat value(float time) const override
        {
            return t_interpolate(keys, *this, time);
        }
        inline vsg::quat operator()(const vsg::quat &a, const vsg::quat &b, float fraction) const
        {
            return vsg::mix(a, b, fraction);
        }
    };
}

#endif
