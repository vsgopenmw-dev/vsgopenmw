#ifndef VSGOPENMW_TERRAIN_BOUNDS_H
#define VSGOPENMW_TERRAIN_BOUNDS_H

#include <vsg/maths/vec2.h>

namespace Terrain
{
    /*
     * Bounds a quadratic region of terrain to be drawn in cell coordinates.
     */
    struct Bounds
    {
        float size = 0;
        static constexpr float minSize = 0.125f;
        vsg::vec2 min;
        bool valid() const { return size > 0.f; }
        vsg::vec2 center() const { return min + vsg::vec2(size*0.5f, size*0.5f); }
        vsg::vec2 max() const { return min + vsg::vec2(size, size); }
        bool contains(const vsg::vec2& point) const
        {
            const vsg::vec2& mincorner = min;
            vsg::vec2 maxcorner = max();
            return (point.x >= mincorner.x && point.x <= maxcorner.x && point.y >= mincorner.y && point.y <= maxcorner.y);
        }
        float radius() const { return size * 0.5f; }
        bool operator == (const Bounds&) const = default;
        bool operator < (const Bounds& rhs) const
        {
            if (size < rhs.size)
                return true;
            else if (size > rhs.size)
                return false;
            return min < rhs.min;
        }
    };

    inline unsigned int ilog2(unsigned int n)
    {
        unsigned int targetlevel = 0;
        while (n >>= 1)
            ++targetlevel;
        return targetlevel;
    }
}

#endif

