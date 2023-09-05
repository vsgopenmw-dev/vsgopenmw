#include "setviewportstate.hpp"

#include <vsg/commands/SetScissor.h>
#include <vsg/commands/SetViewport.h>

namespace vsgUtil
{
    void SetViewportState::record(vsg::CommandBuffer& commandBuffer) const
    {
        vsg::SetViewport(0, viewportState->viewports).record(commandBuffer);
        vsg::SetScissor(0, viewportState->scissors).record(commandBuffer);
    }
}
