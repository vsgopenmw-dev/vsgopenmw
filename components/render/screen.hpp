#ifndef VSGOPENMW_RENDER_SCREEN_H
#define VSGOPENMW_RENDER_SCREEN_H

#include <vsg/maths/mat4.h>

/*
 * Converts to and from screen coordinates.
 */
namespace Render
{
    /*
    vsg::dvec3 screenToWorld(const vsg::dmat4& invViewProj, float nX, float nY, float depth)
    {
        vsg::dvec4 vec = invViewProj * vsg::dvec4(nX * 2 - 1, nY * 2 - 1, depth, 1.0);
        return vsg::dvec3(vec.x, vec.y, vec.z) / vec.w;
    }
    */

    vsg::vec2 worldToScreen(const vsg::dmat4& viewProj, vsg::dvec3 world)
    {
        auto ndc = viewProj * world;
        return { (ndc.x + 1) * 0.5, (ndc.y + 1) * 0.5 };
    }
}

#endif
