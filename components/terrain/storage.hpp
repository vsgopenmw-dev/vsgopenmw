#ifndef COMPONENTS_TERRAIN_STORAGE_H
#define COMPONENTS_TERRAIN_STORAGE_H

#include <vector>

#include <vsg/core/Array.h>
#include <vsg/core/ref_ptr.h>
#include <vsg/maths/vec3.h>

#include "defs.hpp"

namespace Terrain
{
    /// We keep storage of terrain data abstract here since we need different implementations for game and editor
    /// @note The implementation must be thread safe.
    class Storage
    {
    public:
        Storage(float cellWorldSize, unsigned int cellVertices);
        virtual ~Storage() {}

        /// Get bounds of the whole terrain in cell units
        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY) = 0;

        /// Return true if there is land data for this cell
        /// May be overriden for a faster implementation
        virtual bool hasData(int cellX, int cellY)
        {
            float dummy;
            return getMinMaxHeights(1, vsg::vec2(cellX + 0.5, cellY + 0.5), dummy, dummy);
        }

        /// Get the minimum and maximum heights of a terrain region.
        /// @note Will only be called for chunks with size = minBatchSize, i.e. leafs of the quad tree.
        ///        Larger chunks can simply merge AABB of children.
        /// @param size size of the chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param min min height will be stored here
        /// @param max max height will be stored here
        /// @return true if there was data available for this terrain chunk
        virtual bool getMinMaxHeights(float size, const vsg::vec2& center, float& min, float& max) = 0;

        /// Fill vertex buffers for a terrain chunk.
        /// @note May be called from background threads. Make sure to only call thread-safe functions from here!
        /// @note returned colors need to be in render-system specific format! Use RenderSystem::convertColourValue.
        /// @note Vertices should be written in row-major order (a row is defined as parallel to the x-axis).
        ///       The specified positions should be in local space, i.e. relative to the center of the terrain chunk.
        /// @param lodLevel LOD level, 0 = most detailed
        /// @param size size of the terrain chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param positions buffer to write vertices
        /// @param normals buffer to write vertex normals
        /// @param colours buffer to write vertex colours
        virtual void fillVertexBuffers(int lodLevel, float size, const vsg::vec2& center,
            vsg::ref_ptr<vsg::floatArray>& out_heights, vsg::ref_ptr<vsg::vec3Array>& out_normals,
            vsg::ref_ptr<vsg::ubvec4Array>& out_colours)
            = 0;

        /// Create textures holding layer blend values for a terrain chunk.
        /// @note The terrain chunk shouldn't be larger than one cell since otherwise we might
        ///       have to do a ridiculous amount of different layers. For larger chunks, composite maps should be used.
        /// @note May be called from background threads. Make sure to only call thread-safe functions from here!
        /// @param chunkSize size of the terrain chunk in cell units
        /// @param chunkCenter center of the chunk in cell units
        /// @param blendmaps created blendmaps will be written here
        /// @param layerList names of the layer textures used will be written here
        virtual void getBlendmaps(
            float chunkSize, const vsg::vec2& chunkCenter, vsg::ref_ptr<vsg::Data>& blendmaps, std::vector<LayerInfo>& layerList)
            = 0;

        virtual float getHeightAt(const vsg::vec3& worldPos) = 0;

        /// The transformation factor for mapping cell units to world units.
        const float cellWorldSize;

        /// The number of vertices on one side for each cell. Should be (power of two)+1
        const unsigned int cellVertices;
    };

}

#endif
