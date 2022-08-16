#ifndef VSGOPENMW_ANIMATION_RESET_H
#define VSGOPENMW_ANIMATION_RESET_H

#include "channel.hpp"

#include <vsg/maths/vec3.h>

namespace Anim
{
    /*
     * Resets vector components.
     */
    class Reset : public Anim::Channel<vsg::vec3>
    {
    public:
        Reset(vsg::ref_ptr<const Anim::Channel<vsg::vec3>> in_channel) : input(in_channel) {}
        vsg::ref_ptr<const Anim::Channel<vsg::vec3>> input;
        vsg::vec3 resetAxes;
        vsg::vec3 value(float time) const override
        {
            return input->value(time) * resetAxes;
        }
        void setAccumulate(const vsg::vec3 &accumulate)
        {
            // anything that accumulates (1.f) should be reset to (0.f)
            resetAxes.x = accumulate.x != 0.f ? 0.f : 1.f;
            resetAxes.y = accumulate.y != 0.f ? 0.f : 1.f;
            resetAxes.z = accumulate.z != 0.f ? 0.f : 1.f;
        }
    };
}

#endif
