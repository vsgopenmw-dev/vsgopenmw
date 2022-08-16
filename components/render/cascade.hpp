#ifndef VSGOPENMW_RENDER_CASCADE_H
#define VSGOPENMW_RENDER_CASCADE_H

#include <vsg/viewer/Camera.h>

#include "depth.hpp"

namespace Render //namespace vulkanexamples
{
    struct Cascade
    {
        float splitDepth;
        vsg::Orthographic ortho;
        vsg::LookAt lookAt;
    };

    /*
     * Progressively covers view frustum.
     */
    inline std::vector<Cascade> cascade(size_t count, float near, float far, const vsg::Camera &cam, const vsg::vec3 &lightDir)
    {
        using vec3 = vsg::dvec3;
        std::vector<Cascade> cascades(count);
        std::vector<double> cascadeSplits(count);

        float cascadeSplitLambda = 0.95; //if (overlay->sliderFloat("Split lambda", &cascadeSplitLambda, 0.1f, 1.0f)) {
        float clipRange = far - near;

        float minZ = near;
        float maxZ = near + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        // Calculate split depths based on view camera frustum
        // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for (uint32_t i=0; i<count; ++i)
        {
            double p = (i + 1) / static_cast<double>(count);
            double log = minZ * std::pow(ratio, p);
            double uniform = minZ + range * p;
            double d = cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - near) / clipRange;
        }

        // Calculate orthographic projection matrix for each cascade
        double lastSplitDist = 0.0;
        for (uint32_t i=0; i<count; ++i)
        {
            auto splitDist = cascadeSplits[i];

            std::array<vec3,8> frustumCorners = {
                vec3{-1.0, 1.0, nearDepth},
                vec3{1.0, 1.0, nearDepth},
                vec3{1.0, -1.0, nearDepth},
                vec3{-1.0, -1.0, nearDepth},
                vec3{-1.0, 1.0, farDepth},
                vec3{1.0, 1.0, farDepth},
                vec3{1.0, -1.0, farDepth},
                vec3{-1.0, -1.0, farDepth}
            };

            // Project frustum corners into world space
            auto invCam = vsg::inverse_4x3(cam.projectionMatrix->transform() * cam.viewMatrix->transform());
            for (auto &corner : frustumCorners)
            {
                auto invCorner = invCam * vsg::dvec4(corner, 1.0f);
                corner = vec3(invCorner.x, invCorner.y, invCorner.z) / invCorner.w;
            }

            for (uint32_t i=0; i<4; ++i)
            {
                auto dist = frustumCorners[i+4] - frustumCorners[i];
                frustumCorners[i+4] = frustumCorners[i] + (dist * splitDist);
                frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
            }

            // Get frustum center
            auto frustumCenter = vec3(0.0, 0.0, 0.0);
            for (auto &corner : frustumCorners)
                frustumCenter += corner;
            frustumCenter /= 8.0;

            double radius = 0.0;
            for (auto &corner : frustumCorners)
            {
                auto distance = vsg::length(corner - frustumCenter);
                radius = std::max(radius, distance);
            }
            radius = std::ceil(radius * 16.0) / 16.0;

            auto maxExtents = vec3(radius,radius,radius);
            auto minExtents = maxExtents * -1.0;

            // Store split distance and matrix in cascade
            cascades[i].lookAt = vsg::LookAt(frustumCenter + vec3(lightDir) * minExtents.z, frustumCenter, vec3(0.0, 1.0, 0.0));
            cascades[i].ortho = vsg::Orthographic(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0, maxExtents.z - minExtents.z);
            cascades[i].splitDepth = (near + splitDist * clipRange) * -1.0;
            lastSplitDist = cascadeSplits[i];
        }
        return cascades;
    }
}

#endif
