#include "builder.hpp"

#include <vsg/io/Options.h>
#include <vsg/state/Sampler.h>

#include "graphics.hpp"
#include "particle.hpp"

namespace Pipeline
{
    Builder::Builder(vsg::ref_ptr<const vsg::Options> shaderOptions, vsg::ref_ptr<vsg::Sampler> in_defaultSampler)
        : graphics(new Graphics(shaderOptions))
        , particle(new Particle(shaderOptions))
        , defaultSampler(in_defaultSampler)
    {
    }

    Builder::~Builder() {}

    vsg::ref_ptr<vsg::Sampler> Builder::createSampler() const
    {
        return vsg::Sampler::create(*defaultSampler);
    }
}
