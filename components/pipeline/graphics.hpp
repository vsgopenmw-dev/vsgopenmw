#ifndef VSGOPENMW_PIPELINE_GRAPHICS_H
#define VSGOPENMW_PIPELINE_GRAPHICS_H

#include <components/vsgutil/cache.hpp>

#include "options.hpp"

namespace Pipeline
{
    /*
     * Creates general purpose graphics pipelines.
     */
    class Graphics : public vsgUtil::RefCache<Options, vsg::BindGraphicsPipeline>
    {
        vsg::ref_ptr<const vsg::Options> mShaderOptions;
    public:
        Graphics(vsg::ref_ptr<const vsg::Options> readShaderOptions) : mShaderOptions(readShaderOptions) {}
        vsg::ref_ptr<vsg::BindGraphicsPipeline> create(const Options &key) const;
        inline vsg::ref_ptr<vsg::BindGraphicsPipeline> getOrCreate(const Options &key) const
        {
            return vsgUtil::RefCache<Options, vsg::BindGraphicsPipeline>::getOrCreate(key, *this);
        }
    };
}

#endif
