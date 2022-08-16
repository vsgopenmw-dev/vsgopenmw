#include "rotate.hpp"

namespace Anim
{
    vsg::quat Rotate::value(float time) const
    {
        vsg::quat rotate;
        for (auto &[angle, axis] : rotations)
            rotate = rotate * vsg::quat(angle->value(time), axis);
        return rotate;
    }
}
