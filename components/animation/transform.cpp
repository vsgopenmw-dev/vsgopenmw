#include "transform.hpp"

#include <vsg/maths/transform.h>

namespace Anim
{
    void Transform::setAttitude(const vsg::quat &q)
    {
        setRotation(vsg::rotate(q));
    }
}
