#ifndef VSGOPENMW_VSGUTIL_BOX_H
#define VSGOPENMW_VSGUTIL_BOX_H

#include <vsg/maths/box.h>
#include <vsg/maths/sphere.h>

namespace vsgUtil
{
    template <class T>
    vsg::t_vec3<T> corner(unsigned int pos, const vsg::t_box<T> &box)
    {
        return {pos&1?box.max.x:box.min.x,pos&2?box.max.y:box.min.y,pos&4?box.max.z:box.min.z};
    }

    template <class T>
    vsg::t_vec3<T> center(const vsg::t_box<T> &box)
    {
        return (box.min+box.max)/T(2);
    }

    template <class T>
    vsg::t_vec3<T> extent(const vsg::t_box<T> &box)
    {
        return box.max-box.min;
    }

    template<typename T>
    vsg::t_sphere<T> toSphere(const vsg::t_box<T> &box)
    {
        auto radius = vsg::length(extent(box)/T(2));
        return {center(box), radius};
    }
}

#endif
