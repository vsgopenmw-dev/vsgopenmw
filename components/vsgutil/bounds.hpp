#ifndef VSGOPENMW_VSGUTIL_BOUNDS_H
#define VSGOPENMW_VSGUTIL_BOUNDS_H

#include <vsg/maths/box.h>
#include <vsg/maths/sphere.h>

/*
 * Extends box and sphere.
 */
namespace vsgUtil
{
    template <class T>
    vsg::t_vec3<T> corner(unsigned int pos, const vsg::t_box<T>& box)
    {
        return { pos & 1 ? box.max.x : box.min.x, pos & 2 ? box.max.y : box.min.y, pos & 4 ? box.max.z : box.min.z };
    }

    template <class T>
    vsg::t_vec3<T> center(const vsg::t_box<T>& box)
    {
        return (box.min + box.max) / T(2);
    }

    template <class T>
    vsg::t_vec3<T> extent(const vsg::t_box<T>& box)
    {
        return box.max - box.min;
    }

    template <typename T>
    vsg::t_sphere<T> toSphere(const vsg::t_box<T>& box)
    {
        if (!box.valid())
            return {};
        auto radius = vsg::length(extent(box) / T(2));
        return { center(box), radius };
    }

    template <typename T>
    vsg::t_sphere<T> transformSphere(const vsg::t_mat4<T>& matrix, const vsg::t_sphere<T>& in_sphere)
    {
        auto bsphere = in_sphere;
        auto xdash = bsphere.center;
        xdash.x += bsphere.radius;
        xdash = matrix * xdash;

        auto ydash = bsphere.center;
        ydash.y += bsphere.radius;
        ydash = matrix * ydash;

        auto zdash = bsphere.center;
        zdash.z += bsphere.radius;
        zdash = matrix * zdash;

        bsphere.center = matrix * bsphere.center;

        xdash -= bsphere.center;
        auto sqrlen_xdash = vsg::length2(xdash);

        ydash -= bsphere.center;
        auto sqrlen_ydash = vsg::length2(ydash);

        zdash -= bsphere.center;
        auto sqrlen_zdash = vsg::length2(zdash);

        bsphere.radius = sqrlen_xdash;
        if (bsphere.radius < sqrlen_ydash)
            bsphere.radius = sqrlen_ydash;
        if (bsphere.radius < sqrlen_zdash)
            bsphere.radius = sqrlen_zdash;
        bsphere.radius = sqrtf(bsphere.radius);
        return bsphere;
    }

    template<typename T>
    void expandBoxBySphere(vsg::t_box<T>& box, const vsg::t_sphere<T>& sh)
    {
        if (!sh.valid()) return;

        if(sh.center.x-sh.radius<box.min.x) box.min.x = sh.center.x-sh.radius;
        if(sh.center.x+sh.radius>box.max.x) box.max.x = sh.center.x+sh.radius;

        if(sh.center.y-sh.radius<box.min.y) box.min.y = sh.center.y-sh.radius;
        if(sh.center.y+sh.radius>box.max.y) box.max.y = sh.center.y+sh.radius;

        if(sh.center.z-sh.radius<box.min.z) box.min.z = sh.center.z-sh.radius;
        if(sh.center.z+sh.radius>box.max.z) box.max.z = sh.center.z+sh.radius;
    }
}

#endif
