#ifndef VSGOPENMW_TERRAIN_CACHEBUFFER_H
#define VSGOPENMW_TERRAIN_CACHEBUFFER_H

#include <vsg/commands/BindIndexBuffer.h>

#include <components/vsgutil/cache.hpp>

namespace Terrain
{
    struct IndexBufferKey
    {
        unsigned int numVerts;
        unsigned int flags;
        auto operator<=>(const IndexBufferKey&) const = default;
    };

    /// @brief Implements creation and caching of vertex buffers for terrain chunks.
    class CacheBuffer
    {
    public:
        /// @param flags first 4*4 bits are LOD deltas on each edge, respectively (4 bits each)
        ///              next 4 bits are LOD level of the index buffer (LOD 0 = don't omit any vertices)
        /// @note Thread safe.
        vsg::ref_ptr<vsg::BindIndexBuffer> getIndexBuffer (unsigned int numVerts, unsigned int flags);

        /// @note Thread safe.
        vsg::ref_ptr<vsg::Data> getUVBuffer(unsigned int numVerts);
    private:
        vsgUtil::RefCache<IndexBufferKey, vsg::BindIndexBuffer> mIndexBufferCache;
        vsgUtil::RefCache<unsigned int, vsg::Data> mUvBufferCache;
    };
}

#endif
