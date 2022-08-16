#ifndef VSGOPENMW_ANIMATION_ROTATE_H
#define VSGOPENMW_ANIMATION_ROTATE_H

#include <vector>

#include <vsg/maths/quat.h>

#include "channel.hpp"

namespace Anim
{
    class Rotate : public Channel<vsg::quat>
    {
    public:
        vsg::quat value(float time) const override;
        using AngleAxis = std::pair<channel_ptr<float>, vsg::vec3>;
        std::vector<AngleAxis> rotations;
    };
}

#endif
