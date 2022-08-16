#ifndef VSGOPENMW_PIPELINE_WORLDOVERLAY_H
#define VSGOPENMW_PIPELINE_WORLDOVERLAY_H

#include <vsg/state/ComputePipeline.h>

namespace Pipeline
{
    vsg::ref_ptr<vsg::BindComputePipeline> worldOverlay(vsg::ref_ptr<const vsg::Options> shaderOptions);
}

#endif
