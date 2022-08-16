#ifndef VSGOPENMW_PIPELINE_BUILDER_H
#define VSGOPENMW_PIPELINE_BUILDER_H

#include <memory>

#include <vsg/core/ref_ptr.h>

namespace vsg
{
    class Options;
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
        Builder(vsg::ref_ptr<const vsg::Options> shaderOptions);
        ~Builder();
        const std::unique_ptr<Graphics> graphics;
        const std::unique_ptr<Particle> particle;
    };
}

#endif

