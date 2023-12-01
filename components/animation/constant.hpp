#ifndef VSGOPENMW_ANIMATION_CONSTANT_H
#define VSGOPENMW_ANIMATION_CONSTANT_H

#include "channel.hpp"

namespace Anim
{
    /*
     * Constant<T> is a channel class that returns a fixed value and avoids all calculation.
     * Typically used to assign default values to unused channels, and to avoid the need for checking the validity of channels in Controller::run(..) implementations.
     */
    template <typename T>
    class Constant : public Channel<T>
    {
    public:
        T val;
        Constant(const T& v)
            : val(v)
        {
        }
        T value(float time) const override { return val; }
    };

    template <typename T>
    channel_ptr<T> make_constant(const T& val)
    {
        return channel_ptr<T>(new Constant(val));
    }
}

#endif
