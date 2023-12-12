#include "rotate.hpp"

#include <vsg/maths/transform.h>

#include <components/vsgutil/computetransform.hpp>
#include <components/vsgutil/transform.hpp>

#include "bones.hpp"
#include "transform.hpp"

namespace Anim
{
    vsg::quat Rotate::value(float time) const
    {
        vsg::quat rotate;
        for (auto& [angle, axis] : rotations)
            rotate = rotate * vsg::quat(angle->value(time), axis);
        return rotate;
    }

    void rotateBone(Bone& bone, const vsg::mat4& rotate, const vsg::vec3& offset)
    {
        auto& t = *bone.node();
        vsg::mat4 nodeMatrix = t.t_transform(vsg::mat4());
        auto worldMat = vsgUtil::computeTransform(bone.path);
        auto worldMatInv = vsg::inverse_4x3(worldMat);
        auto mat = nodeMatrix * worldMatInv * rotate * worldMat;
        t.setRotation(mat);
        t.translation += vsgUtil::transform3x3(worldMatInv, offset);
    }
}
