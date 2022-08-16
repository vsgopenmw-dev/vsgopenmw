#ifndef VSGOPENMW_VSGADAPTERS_OSGCOMPAT_H
#define VSGOPENMW_VSGADAPTERS_OSGCOMPAT_H

#include <vsg/maths/vec3.h>
#include <vsg/maths/vec4.h>
#include <vsg/maths/quat.h>
#include <vsg/maths/mat4.h>
#include <vsg/core/Array.h>

#include <osg/Vec3>
#include <osg/Vec3>
#include <osg/Quat>
#include <osg/Matrixf>

//namespace
//{
    inline vsg::quat toVsg(const osg::Quat &quat)
    {
        return vsg::quat(quat.x(), quat.y(), quat.z(), quat.w());
    }
    inline vsg::vec2 toVsg(const osg::Vec2f &vec)
    {
        return {vec.x(), vec.y()};
    }
    inline vsg::vec3 toVsg(const osg::Vec3f &vec)
    {
        return {vec.x(), vec.y(), vec.z()};
    }
    inline vsg::vec4 toVsg(const osg::Vec4f &vec)
    {
        return {vec.x(), vec.y(), vec.z(), vec.w()};
    }
    inline vsg::dvec3 toVsg(const osg::Vec3d &vec)
    {
        return {vec.x(), vec.y(), vec.z()};
    }
    inline float toVsg(float val)
    {
        return val;
    }
    template <class Array, class Source>
    vsg::ref_ptr<Array> copyArray(const Source &vec)
    {
        vsg::ref_ptr<Array> array = Array::create(vec.size());
        std::memcpy(array->data(), &vec.front(), vec.size()*sizeof(typename Source::value_type));
        return array;
    }

    inline osg::Quat toOsg(const vsg::quat &quat)
    {
        return osg::Quat(quat.x, quat.y, quat.z, quat.w);
    }
    inline osg::Vec3f toOsg(const vsg::vec3 &vec)
    {
        return {vec.x, vec.y, vec.z};
    }
    inline osg::Vec4f toOsg(const vsg::vec4 &vec)
    {
        return {vec.x, vec.y, vec.z, vec.w};
    }
    inline osg::Matrixf toOsg(vsg::mat4 m)
    {
        osg::Matrixf osg;
        std::memcpy(&osg, &m, sizeof(m));
        return osg;
    }
//}

#endif
