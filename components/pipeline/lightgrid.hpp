#ifndef VSGOPENMW_PIPELINE_LIGHTGRID_H
#define VSGOPENMW_PIPELINE_LIGHTGRID_H

#include <vsg/state/ComputePipeline.h>

namespace Pipeline
{
    vsg::ref_ptr<vsg::BindComputePipeline> lightGrid(vsg::ref_ptr<const vsg::Options> shaderOptions);
}

#endif
