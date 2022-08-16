#ifndef VSGOPENMW_ANIMATION_CONSTANT_H
#define VSGOPENMW_ANIMATION_CONSTANT_H

#include "channel.hpp"

namespace Anim
{
    /*
     * Shortcuts calculations.
     */
    template <typename T>
    class Constant : public Channel<T>
    {
    public:
        T val;
        Constant(const T &v) : val(v) {}
        T value(float time) const override
        {
            return val;
        }
    };
}

#endif
