#ifndef VSGOPENMW_VSGUTIL_TRANSFORM_H
#define VSGOPENMW_VSGUTIL_TRANSFORM_H

#include <vsg/maths/mat3.h>
#include <vsg/maths/mat4.h>
#include <vsg/maths/vec3.h>

namespace vsgUtil
{
    /*
     * Optimized version of lhsMatrix * vec4(rhsVector, 0), typically used for transforming direction vectors.
     */
    template<typename T>
    vsg::t_vec3<T> transform3x3(const vsg::t_mat4<T>& lhs, const vsg::t_vec3<T>& rhs)
    {
        return vsg::t_vec3<T>((lhs[0][0] * rhs[0] + lhs[1][0] * rhs[1] + lhs[2][0] * rhs[2]),
            (lhs[0][1] * rhs[0] + lhs[1][1] * rhs[1] + lhs[2][1] * rhs[2]),
            (lhs[0][2] * rhs[0] + lhs[1][2] * rhs[1] + lhs[2][2] * rhs[2]));
    }

    /*
     * Optimized version of vec4(lhsVector, 0) * rhsMatrix, typically used to avoid a transpose(..) call when transforming normals by an inverse transpose normal matrix.
     */
    template<typename T>
    vsg::t_vec3<T> transform3x3(const vsg::t_vec3<T>& lhs, const vsg::t_mat4<T>& rhs)
    {
        return vsg::t_vec3<T>(lhs[0] * rhs[0][0] + lhs[1] * rhs[0][1] + lhs[2] * rhs[0][2],
            lhs[0] * rhs[1][0] + lhs[1] * rhs[1][1] + lhs[2] * rhs[1][2],
            lhs[0] * rhs[2][0] + lhs[1] * rhs[2][1] + lhs[2] * rhs[2][2]);
    }

    /*
     * Gets the rotation part of a 4x4 matrix.
     */
    template<typename T>
    vsg::t_mat3<T> getRotation(const vsg::t_mat4<T>& mat)
    {
        return {
            mat[0][0], mat[0][1], mat[0][2],
            mat[1][0], mat[1][1], mat[1][2],
            mat[2][0], mat[2][1], mat[2][2]
        };
    }

    /*
     * Sets the rotation part of a 4x4 matrix.
     */
    template<typename T>
    void setRotation(vsg::t_mat4<T>& mat, const vsg::t_mat3<T>& rot)
    {
        mat = {
            vsg::t_vec4<T>(rot[0], mat[0][3]),
            vsg::t_vec4<T>(rot[1], mat[1][3]),
            vsg::t_vec4<T>(rot[2], mat[2][3]),
            mat[3],
        };
    }
}

#endif
