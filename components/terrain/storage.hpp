#ifndef VSGOPENMW_TERRAIN_STORAGE_H
#define VSGOPENMW_TERRAIN_STORAGE_H

#include <vector>
#include <string>

#include <vsg/core/Array2D.h>
#include <vsg/core/ref_ptr.h>
#include <vsg/maths/vec3.h>

#include <components/esm/refid.hpp>

namespace Terrain
{
    class Bounds;

    using LayerInfo = std::string;

    /// We keep storage of terrain data abstract here since we need different implementations for game and editor
    /// @note The const implementation must be thread safe.
    class Storage
    {
    public:
        Storage(float in_cellWorldSize, uint32_t in_cellVertices) : cellWorldSize(in_cellWorldSize), cellVertices(in_cellVertices) {}
        virtual ~Storage() {}

        /// Fill vertex buffers for a terrain chunk.
        /// @param lodLevel LOD level, 0 = most detailed
        struct VertexData
        {
            vsg::ref_ptr<vsg::floatArray2D> heights;
            vsg::ref_ptr<vsg::vec3Array2D> normals;
            vsg::ref_ptr<vsg::ubvec4Array2D> colors;
        };
        virtual VertexData getVertexData(int lodLevel, const Bounds& bounds) = 0;

        /// Create textures holding layer blend values for a terrain chunk.
        virtual vsg::ref_ptr<vsg::Data> getBlendmap(const Bounds& bounds) = 0;

        virtual std::vector<LayerInfo> getLayers() = 0;

        virtual float getHeightAt(const vsg::vec3& worldPos, ESM::RefId worldspace) = 0;

        /// The transformation factor for mapping cell units to world units.
        const float cellWorldSize;

        /// The number of vertices on one side for each cell. Should be (power of two)+1
        const uint32_t cellVertices;

        uint32_t numVerts(float chunkSize, uint8_t lod) const
        {
            return (cellVertices - 1) * chunkSize / (1 << lod) + 1;
        }
    };
}

#endif
