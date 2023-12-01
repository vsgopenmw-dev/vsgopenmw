#ifndef VSGOPENMW_VSGUTIL_DEBUGBARRIER_H
#define VSGOPENMW_VSGUTIL_DEBUGBARRIER_H

#include <vsg/commands/PipelineBarrier.h>

namespace vsgUtil
{
    vsg::ref_ptr<vsg::PipelineBarrier> debugBarrier()
    {
        return vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
            vsg::MemoryBarrier::create(VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT));
    }
}

#endif
