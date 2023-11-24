#include "sharedview.hpp"

#include <vsg/nodes/Bin.h>
#include <vsg/state/DynamicState.h>
#include <vsg/state/ViewDependentState.h>

#include "setviewportstate.hpp"

namespace vsgUtil
{
    vsg::ref_ptr<vsg::View> createSharedView()
    {
        static const auto baseView = vsg::View::create(); // obtainSharedViewID()
        auto view = vsg::View::create(*baseView); // copyViewID()
        view->viewDependentState = nullptr;
        view->overridePipelineStates
            = { vsg::DynamicState::create(VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR) };
        return view;
    }

    vsg::ref_ptr<vsg::View> createSharedView(vsg::ref_ptr<vsg::Camera> camera, vsg::ref_ptr<vsg::Node> child)
    {
        auto view = createSharedView();
        view->camera = camera;
        view->children = { vsgUtil::SetViewportState::create(camera->viewportState), child };
        return view;
    }
}
