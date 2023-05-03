#ifndef VSGOPENMW_PIPELINE_GRAPHICS_H
#define VSGOPENMW_PIPELINE_GRAPHICS_H

#include <vsg/state/GraphicsPipeline.h>

#include <components/vsgutil/cache.hpp>

#include "options.hpp"

namespace vsg
{
    class SharedObjects;
}
namespace Pipeline
{
    /*
     * Creates general purpose graphics pipelines.
     */
    class Graphics
    {
        vsg::ref_ptr<const vsg::Options> mShaderOptions;
        vsg::ref_ptr<vsg::SharedObjects> mSharedObjects;
        vsgUtil::RefCache<Options, vsg::BindGraphicsPipeline> mPipelineCache;
        class VertexStageOptions : public ShaderOptions{};
        class FragmentStageOptions : public ShaderOptions{};
        vsgUtil::RefCache<VertexStageOptions, vsg::ShaderStage> mVertexStageCache;
        vsgUtil::RefCache<FragmentStageOptions, vsg::ShaderStage> mFragmentStageCache;
        // mSpecializationCache;

    public:
        Graphics(vsg::ref_ptr<const vsg::Options> readShaderOptions);
        ~Graphics();

        vsg::ref_ptr<vsg::BindGraphicsPipeline> create(const Options& key) const;
        vsg::ref_ptr<vsg::ShaderStage> create(const VertexStageOptions& key) const;
        vsg::ref_ptr<vsg::ShaderStage> create(const FragmentStageOptions& key) const;
        vsg::ref_ptr<vsg::ShaderStage> createShaderStage(const ShaderOptions& key, const std::string& suffix) const;
        inline vsg::ref_ptr<vsg::BindGraphicsPipeline> getOrCreate(const Options& key) const
        {
            return mPipelineCache.getOrCreate(key, *this);
        }
    };
}

#endif
