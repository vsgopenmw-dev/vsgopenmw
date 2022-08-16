#ifndef VSGOPENMW_VIEW_BARRIERS_H
#define VSGOPENMW_VIEW_BARRIERS_H

#include <vsg/commands/PipelineBarrier.h>

namespace View
{
    //vkQueueSubmit-implicit-host-sync
    //VUID-
    /*
    vsg::ref_ptr<vsg::PipelineBarrier> hostWriteBarrier()
    {
        return vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_SHADER_STAGE_VERTEX_BIT, 0, vsg::MemoryBarrier::create(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_UNIFORM_READ_BIT));
    }
    */
    vsg::ref_ptr<vsg::PipelineBarrier> computeBarrier()
    {
        return vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, vsg::MemoryBarrier::create(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT));
    }
}

#endif
