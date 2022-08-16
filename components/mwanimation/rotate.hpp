#ifndef VSGOPENMW_MWANIMATION_ROTATE_H
#define VSGOPENMW_MWANIMATION_ROTATE_H

#include <vsg/maths/mat4.h>

namespace Anim
{
    class Bone;
}
namespace MWAnim
{
    /// Applies a rotation in referenceFrame.
    /// @note Assumes that the node being rotated has its "original" orientation set every frame.
    /// The rotation is then applied on top of that orientation.
    void rotate(Anim::Bone &bone, const vsg::mat4 &rotate, const vsg::vec3 &offset = {});
}

#endif
