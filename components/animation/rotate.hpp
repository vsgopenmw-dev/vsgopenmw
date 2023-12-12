#ifndef VSGOPENMW_ANIMATION_ROTATE_H
#define VSGOPENMW_ANIMATION_ROTATE_H

#include <vector>

#include <vsg/maths/quat.h>
#include <vsg/maths/mat4.h>

#include "channel.hpp"

namespace Anim
{
    /// Channel for converting a list of angle/axis rotations to a quaternion value.
    class Rotate : public Channel<vsg::quat>
    {
    public:
        vsg::quat value(float time) const override;
        using AngleAxis = std::pair<channel_ptr<float>, vsg::vec3>;
        std::vector<AngleAxis> rotations;
    };

    /// Applies a rotation in skeleton reference frame.
    /// @note Assumes that the node being rotated has its "original" orientation set every frame.
    /// The rotation is then applied on top of that orientation.
    class Bone;
    void rotateBone(Bone& bone, const vsg::mat4& rotate, const vsg::vec3& offset = {});

}

#endif
