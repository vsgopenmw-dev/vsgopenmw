#ifndef VSGOPENMW_ANIMATION_CHANNEL_H
#define VSGOPENMW_ANIMATION_CHANNEL_H

#include <vsg/core/Object.h>

namespace Anim
{
    /*
     * Computes animation values as a function of time.
     */
    template <typename T>
    class Channel : public vsg::Object
    {
    public:
        virtual T value(float time) const = 0;
    };

    template <typename T>
    using channel_ptr = vsg::ref_ptr<Channel<T>>;

    template <typename T, typename ... Args>
    vsg::ref_ptr<T> make_channel(Args&&... args)
    {
        return vsg::ref_ptr<T>(new T(args...));
    }
}

#endif
