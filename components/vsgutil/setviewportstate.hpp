#ifndef VSGOPENMW_VSGUTIL_SETVIEWPORTSTATE_H
#define VSGOPENMW_VSGUTIL_SETVIEWPORTSTATE_H

#include <vsg/commands/Command.h>

namespace vsgUtil
{
    /*
     * Supports VkDynamicState pipelines.
     */
    class SetViewportState : public vsg::Inherit<vsg::Command, SetViewportState>
    {
    public:
        SetViewportState(vsg::ref_ptr<vsg::ViewportState> v) : viewportState(v) {}
        vsg::ref_ptr<vsg::ViewportState> viewportState;
        void record(vsg::CommandBuffer& commandBuffer) const override;
    };
}

#endif
