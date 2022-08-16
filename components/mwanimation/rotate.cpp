#include "rotate.hpp"

#include <vsg/maths/transform.h>

#include <components/animation/transform.hpp>
#include <components/animation/bones.hpp>
#include <components/vsgutil/computetransform.hpp>

namespace MWAnim
{
    void rotate(Anim::Bone &bone, const vsg::mat4 &rotate, const vsg::vec3 &offset)
    {
        auto &t = *bone.node();
        vsg::mat4 nodeMatrix = t.t_transform(vsg::mat4());
        auto worldMat = vsgUtil::computeTransform(bone.path);
        auto worldMatInv = vsg::inverse_4x3(worldMat);
        auto mat = nodeMatrix * worldMatInv * rotate * worldMat;
        t.setRotation(mat);
        worldMatInv[3] = {0,0,0,1};
        t.translation += worldMatInv * offset;
    }
}
