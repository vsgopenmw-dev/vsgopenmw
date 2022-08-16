#ifndef VSGOPENMW_ANIMATION_PATH_H
#define VSGOPENMW_ANIMATION_PATH_H

#include <vsg/maths/vec3.h>

#include "channel.hpp"

namespace Anim
{
    /*
     * Moves position along a path.
     */
    class Path : public Channel<vsg::vec3>
    {
    public:
        vsg::vec3 value(float time) const override
        {
            float p = percent->value(time);
            if (p < 0.f)
                p = std::fmod(p, 1.f) + 1.f;
            else if (p > 1.f)
                p = std::fmod(p, 1.f);
            return path->value(p);
        }
        channel_ptr<float> percent;
        channel_ptr<vsg::vec3> path;
    };
}

#endif
