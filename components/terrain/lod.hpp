#ifndef VSGOPENMW_TERRAIN_LOD_H
#define VSGOPENMW_TERRAIN_LOD_H

#include "bounds.hpp"

namespace Terrain
{
    unsigned int lodLevel(float lodValue)
    {
        return ilog2(static_cast<unsigned int>(lodValue));
    }

    float lodValue(float size)
    {
        return size / Bounds::minSize;
    }

    /// get the level of vertex detail to render this node at, expressed relative to the native resolution of the vertex
    /// data set, NOT relative to minSize as is the case with node LODs.
    unsigned int getVertexLod(float chunkSize, int vertexLodMod)
    {
        unsigned int vertexLod = lodLevel(chunkSize);
        if (vertexLodMod > 0)
        {
            vertexLod = static_cast<unsigned int>(std::max(0, static_cast<int>(vertexLod) - vertexLodMod));
        }
        else if (vertexLodMod < 0)
        {
            float size = chunkSize;
            // Stop to simplify at this level since with size = 1 the node already covers the whole cell and has
            // getCellVertices() vertices.
            while (size < 1)
            {
                size *= 2;
                vertexLodMod = std::min(0, vertexLodMod + 1);
            }
            vertexLod += std::abs(vertexLodMod);
        }
        return vertexLod;
    }

    bool isSufficientDetail(float chunkSize, float dist, float factor)
    {
        if (chunkSize <= Bounds::minSize)
            return true;
        return lodLevel(lodValue(chunkSize)) <= lodLevel(lodValue(dist) / factor);
    }

    float distance(const Bounds& bounds, const vsg::dvec2& eye)
    {
        const vsg::vec2& min = bounds.min;
        vsg::vec2 max = bounds.max();
        vsg::vec2 maxDist;
        if (eye.x < min.x)
            maxDist.x = min.x - eye.x;
        else if (eye.x > max.x)
            maxDist.x = max.x - eye.x;
        if (eye.y < min.y)
            maxDist.y = min.y - eye.y;
        else if (eye.y > max.y)
            maxDist.y = max.y - eye.y;
        return vsg::length(maxDist);
    }
}

#endif
