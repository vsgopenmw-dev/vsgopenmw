#ifndef VSGOPENMW_PIPELINE_FOGOFWAR_H
#define VSGOPENMW_PIPELINE_FOGOFWAR_H

#include <vsg/state/ComputePipeline.h>

namespace Pipeline
{
    vsg::ref_ptr<vsg::BindComputePipeline> fogOfWar(vsg::ref_ptr<const vsg::Options> shaderOptions);
}

#endif
