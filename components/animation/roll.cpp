#include "roll.hpp"

#include <vsg/maths/transform.h>

namespace Anim
{
    void Roll::apply(Transform& transform, float time)
    {
        vsg::mat4 matrix;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                matrix(j, i) = transform.rotation(j, i);
        matrix = matrix * vsg::rotate(speed->value(time) * mDt.get(time), axis);
        transform.setRotation(matrix);
    }
}
