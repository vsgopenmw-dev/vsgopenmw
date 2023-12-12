#ifndef VSGOPENMW_MWRENDER_INTERSECT_H
#define VSGOPENMW_MWRENDER_INTERSECT_H

#include "rendermanager.hpp"
#include "scene.hpp"

#include <vsg/core/visit.h>

#include <components/vsgutil/intersectnormal.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/pipeline/mode.hpp>
#include <components/animation/transform.hpp>
#include <components/vsgutil/cameradistanceintersector.hpp>

/*
 * Intersects game objects.
 */
namespace MWRender
{
    RenderManager::RayResult getIntersectionResult(
        vsg::LineSegmentIntersector& intersector, Scene& scene)
    {
        auto& intersections = intersector.intersections;
        if (intersections.empty())
            return {};
        std::sort(
            intersections.begin(), intersections.end(),
            [](auto& lhs, auto& rhs) -> auto{ return lhs->ratio < rhs->ratio; });

        const auto& intersection = intersections.front();
        vsg::vec3 nWorld = vsgUtil::worldNormal(*intersection, static_cast<int>(Pipeline::Mode::NORMAL));
        /*
        auto dir = vsg::normalize(eye - vsg::dvec3(toVsg(result.mHitPointWorld)));
        if (vsg::length2(nWorld) > 0 && vsg::dot(dir, vsg::dvec3(nWorld)) < 0)
            continue;
            */

        RenderManager::RayResult result{ .mHit = true, .mHitNormalWorld = toOsg(nWorld), .mHitPointWorld = toOsg(vsg::vec3(intersection->worldIntersection)), .mRatio = static_cast<float>(intersection->ratio) };
        for (auto node : intersection->nodePath)
        {
            if (auto trans = dynamic_cast<const Anim::Transform*>(node))
            {
                result.mHitObject = scene.getPtr(trans);
                break;;
            }
        }
        return result;
    }

    void configure(vsg::Intersector& visitor, bool ignorePlayer, bool ignoreActors)
    {
        visitor.traversalMask = Mask_Terrain|Mask_Object;
        if (!ignoreActors)
            visitor.traversalMask |= Mask_Actor;
        if (!ignorePlayer)
            visitor.traversalMask |= Mask_Player;
    }

    RenderManager::RayResult castRay(
        const vsg::dvec3& origin, const vsg::dvec3& dest, bool ignorePlayer, bool ignoreActors, Scene& scene, vsg::ref_ptr<vsg::Node> node)
    {
        vsg::LineSegmentIntersector intersector(origin, dest);
        configure(intersector, ignorePlayer, ignoreActors);
        return getIntersectionResult(vsg::visit(intersector, node), scene);
    }

    RenderManager::RayResult castCameraToViewportRay(
        float nX, float nY, float maxDistance, bool ignorePlayer, bool ignoreActors, const vsg::Camera& camera, Scene& scene, vsg::ref_ptr<vsg::Node> node)
    {
        vsgUtil::CameraDistanceIntersector intersector(camera, nX, nY, static_cast<double>(maxDistance));
        configure(intersector, ignorePlayer, ignoreActors);
        return getIntersectionResult(vsg::visit(intersector, node), scene);
    }
}

#endif
