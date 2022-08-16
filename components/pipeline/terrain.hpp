#ifndef VSGOPENMW_PIPELINE_TERRAIN_H
#define VSGOPENMW_PIPELINE_TERRAIN_H

#include <vsg/state/GraphicsPipeline.h>
#include <vsg/io/Options.h>
#include <components/vsgutil/cache.hpp>

namespace Pipeline
{
    struct TerrainKey
    {
        uint32_t layerCount = 0;
        auto operator<=>(const TerrainKey&) const = default;
    };

    class Terrain
    {
        vsg::ref_ptr<const vsg::Options> mShaderOptions;
        vsgUtil::RefCache<TerrainKey, vsg::BindGraphicsPipeline> mPipelineCache;
    public:
        Terrain(vsg::ref_ptr<const vsg::Options> shaderOptions) : mShaderOptions(shaderOptions) {}
        vsg::ref_ptr<vsg::BindGraphicsPipeline> create(const TerrainKey &key) const;
        vsg::ref_ptr<vsg::BindGraphicsPipeline> getPipeline(const TerrainKey &key) const
        {
            return mPipelineCache.getOrCreate(key, *this);
        }
    };
}

#endif
