#ifndef VSGOPENMW_TERRAIN_BUILDER_H
#define VSGOPENMW_TERRAIN_BUILDER_H

#include <vsg/nodes/StateGroup.h>
#include <vsg/core/Array2D.h>

#include <components/vsgutil/cache.hpp>

#include "storage.hpp"
#include "indexbuffer.hpp"
#include "bounds.hpp"

namespace vsg
{
    class BindGraphicsPipeline;
}
namespace Terrain
{
    /*
     * Assembles a complete subgraph capable of drawing terrain.
     */
    struct Builder
    {
        Builder(vsg::ref_ptr<const vsg::Options> in_imageOptions, vsg::ref_ptr<const vsg::Options> in_shaderOptions, vsg::ref_ptr<vsg::Sampler> in_samplerOptions);
        ~Builder();

        void pruneCache() const;

        void setStorage(Storage* in_storage);

        using IndexKey = std::pair<uint32_t /*numVerts*/, uint32_t /*flags*/>;
        vsgUtil::RefCache<IndexKey, vsg::BindIndexBuffer> indexCache;

        using Key = std::pair<Bounds, uint8_t /*lod*/>;
        struct BatchData
        {
            vsg::StateGroup::StateCommands stateCommands;
            vsg::ref_ptr<vsg::vec2Array2D> heightRanges;
        };
        vsgUtil::Cache<Key, BatchData> dataCache;

        BatchData create(const Key& key) const;
        BatchData getBatchData(const Bounds& bounds, uint8_t lod = 0) const;

        struct Batch
        {
            Bounds bounds;
            uint8_t lod = 0;
            BatchData data;
            vsg::ref_ptr<vsg::Node> root;
            vsg::ref_ptr<vsg::Group> leafGroup;
        };
        Batch createBatch(const Bounds& bounds, uint8_t lod = 0) const;

        vsg::ref_ptr<vsg::Node> createDraw(const Batch& batch, const Bounds& bounds, uint8_t lod = 0, uint32_t lodFlags = 0) const;
    private:
        vsg::ref_ptr<vsg::Node> createCullNode(vsg::ref_ptr<vsg::Node> child, const Bounds& batchBounds, const BatchData& data, const Bounds& bounds) const;

        vsg::ref_ptr<vsg::BindGraphicsPipeline> mPipeline;
        Storage* mStorage = nullptr;                vsg::ref_ptr<const vsg::Options> mImageOptions;
        vsg::ref_ptr<vsg::Sampler> mSamplerOptions;
        vsg::ref_ptr<vsg::Descriptor> mLayerDescriptor;
    };
}

#endif
