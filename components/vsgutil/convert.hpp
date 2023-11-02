#ifndef VSGOPENMW_VSGUTIL_CONVERT_H
#define VSGOPENMW_VSGUTIL_CONVERT_H

#include <vsg/maths/vec4.h>
#include <vsg/maths/vec3.h>
#include <vsg/maths/vec2.h>

namespace vsgUtil
{
    template <class T>
    vsg::t_vec3<T> toVec3(const vsg::t_vec4<T>& v)
    {
        return { v[0], v[1], v[2] };
    }

    template <class T>
    vsg::t_vec2<T> toVec2(const vsg::t_vec3<T>& v)
    {
        return { v[0], v[1] };
    }

    template <class T>
    vsg::t_vec2<T> toVec2(const vsg::t_vec4<T>& v)
    {
        return { v[0], v[1] };
    }
}

#endif
