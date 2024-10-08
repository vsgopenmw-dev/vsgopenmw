#ifndef VSGOPENMW_VSGUTIL_INTERSECTNORMAL_H
#define VSGOPENMW_VSGUTIL_INTERSECTNORMAL_H

#include <vsg/maths/transform.h>
#include <vsg/utils/LineSegmentIntersector.h>

#include "id.hpp"

namespace vsgUtil
{
    vsg::vec3 worldNormal(const vsg::LineSegmentIntersector::Intersection& intersection, int arrayID)
    {
        for (auto& data : intersection.arrays)
        {
            if (auto id = vsgUtil::ID::get(*data))
            {
                if (id->id == arrayID)
                {
                    auto& array = static_cast<vsg::vec3Array&>(*data);
                    vsg::dvec3 n(0, 0, 0);
                    for (auto& r : intersection.indexRatios)
                        n += vsg::dvec3(array[r.index]) * r.ratio;
                    return vsg::vec3(vsg::normalize(n * vsg::inverse_3x3(intersection.localToWorld)));
                }
            }
        }
        return { 0, 0, 1 };
    }
}

#endif
