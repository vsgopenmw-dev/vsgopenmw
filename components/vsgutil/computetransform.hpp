#ifndef VSGOPENMW_VSGUTIL_COMPUTETRANSFORM_H
#define VSGOPENMW_VSGUTIL_COMPUTETRANSFORM_H

#include <vsg/maths/mat4.h>

#include "convert.hpp"

namespace vsgUtil
{
    /*
     * Provides single precision devirtualized vsg::computeTransform.
     */
    template <class Nodes>
    inline vsg::mat4 computeTransform(const Nodes& nodes)
    {
        vsg::mat4 mat;
        for (auto& node : nodes)
            mat = node->t_transform(mat);
        return mat;
    }

    /*
     * Gets the translation part of a 4x4 matrix.
     */
    template <class T>
    vsg::t_vec3<T> translation(const vsg::t_mat4<T> matrix)
    {
        return vsgUtil::toVec3(matrix[3]);
    }

    template <class Nodes>
    inline vsg::vec3 computePosition(const Nodes& nodes)
    {
        return vsgUtil::translation(vsgUtil::computeTransform(nodes));
    }
}

#endif
