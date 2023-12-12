#include "roll.hpp"

#include <vsg/maths/transform.h>

#include <components/vsgutil/transform.hpp>

namespace Anim
{
    void Roll::apply(Transform& transform, float time)
    {
        transform.rotation = transform.rotation * vsgUtil::getRotation(vsg::rotate(speed->value(time) * mDt.get(time), axis));
    }
}
