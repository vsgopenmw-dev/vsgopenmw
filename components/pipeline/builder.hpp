#ifndef VSGOPENMW_PIPELINE_BUILDER_H
#define VSGOPENMW_PIPELINE_BUILDER_H

#include <memory>

#include <vsg/core/ref_ptr.h>

namespace vsg
{
    class Options;
    class Sampler;
}
namespace Pipeline
{
    class Graphics;
    class Particle;

    /*
     * Builds pipeline state.
     */
    class Builder
    {
        vsg::ref_ptr<const vsg::Options> mShaderOptions;

    public:
        Builder(vsg::ref_ptr<const vsg::Options> in_shaderOptions, vsg::ref_ptr<vsg::Sampler> in_defaultSampler);
        ~Builder();
        vsg::ref_ptr<vsg::Sampler> createSampler() const;

        const std::unique_ptr<Graphics> graphics;
        const std::unique_ptr<Particle> particle;
        vsg::ref_ptr<const vsg::Sampler> defaultSampler;
    };
}

#endif
