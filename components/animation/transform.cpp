#include "transform.hpp"

#include <vsg/io/ObjectFactory.h>
#include <vsg/maths/transform.h>

namespace Anim
{
    static const vsg::RegisterWithObjectFactoryProxy<Transform> registerWithObjectFactory = {};

    void Transform::setAttitude(const vsg::quat& q)
    {
        setRotation(vsg::rotate(q));
    }

    void Transform::read(vsg::Input& input)
    {
        input.read("translation", translation);
        input.read("rotation", rotation);
        input.read("scale", scale);
        vsg::Transform::read(input);
    }

    void Transform::write(vsg::Output& output) const
    {
        output.write("translation", translation);
        output.write("rotation", rotation);
        output.write("scale", scale);
        vsg::Transform::write(output);
    }
}
