#ifndef VSGOPENMW_ANIMATION_CHANNEL_H
#define VSGOPENMW_ANIMATION_CHANNEL_H

#include <vsg/core/Object.h>

namespace Anim
{
    /*
     * Channel is an abstract base class/interface class for computing template animation values as a function of time, usually implemented with a keyframe track.
     * Keeping it abstract allows for other uses, like composite channels that alter the value of an input channel or the time passed into it.
     */
    template <typename T>
    class Channel : public vsg::Object
    {
    public:
        virtual T value(float time) const = 0;
    };

    template <typename T>
    using channel_ptr = vsg::ref_ptr<Channel<T>>;

    /*
     * Convenience function that mirrors vsg::Object::create(..) functionality for objects not derived from vsg::Inherit.
     */
    template <typename T, typename ... Args>
    vsg::ref_ptr<T> make_channel(Args&&... args)
    {
        return vsg::ref_ptr<T>(new T(args...));
    }
}

#endif
