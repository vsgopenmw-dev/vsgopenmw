#ifndef VSGOPENMW_VSGUTIL_COMPUTETRANSFORM_H
#define VSGOPENMW_VSGUTIL_COMPUTETRANSFORM_H

#include <vsg/maths/mat4.h>

namespace vsgUtil
{
    /*
     * Provides single precision devirtualized vsg::computeTransform.
     */
    template <class Nodes>
    inline vsg::mat4 computeTransform(const Nodes &nodes)
    {
        vsg::mat4 mat;
        for (auto &node : nodes)
            mat = node->t_transform(mat);
        return mat;
    }

    template <class Nodes>
    inline vsg::vec3 computePosition(const Nodes &nodes)
    {
        auto vec = vsgUtil::computeTransform(nodes)[3];
        return {vec.x, vec.y, vec.z};
    }
}

#endif
