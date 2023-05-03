#ifndef VSGOPENMW_VSGUTIL_CAMERADISTANCEINTERSECTOR_H
#define VSGOPENMW_VSGUTIL_CAMERADISTANCEINTERSECTOR_H

#include <vsg/utils/LineSegmentIntersector.h>

namespace vsgUtil
{
    /*
     * Adds maximum distance to LineSegmentIntersector(Camera).
     */
    class CameraDistanceIntersector : public vsg::LineSegmentIntersector
    {
    public:
        CameraDistanceIntersector(const vsg::Camera& camera, float nX, float nY, double maxDistance)
            : vsg::LineSegmentIntersector(camera, static_cast<int32_t>(nX * camera.getViewport().width), static_cast<int32_t>(nY * camera.getViewport().height))
        {
            for (auto& segment : _lineSegmentStack)
                segment.end = segment.start + vsg::normalize(segment.end - segment.start) * maxDistance;
        }
    };
}

#endif
