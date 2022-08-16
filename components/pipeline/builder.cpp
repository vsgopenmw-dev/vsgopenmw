#include "builder.hpp"

#include <vsg/io/Options.h>

#include "graphics.hpp"
#include "particle.hpp"

namespace Pipeline
{
    Builder::Builder(vsg::ref_ptr<const vsg::Options> shaderOptions)
        : graphics(new Graphics(shaderOptions))
        , particle(new Particle(shaderOptions))
    {
    }

    Builder::~Builder()
    {
    }
};
