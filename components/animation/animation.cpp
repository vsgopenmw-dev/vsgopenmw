#include "animation.hpp"

#include "controller.hpp"

namespace Anim
{
    void Animation::link(Context& ctx, std::function<void(const Controller*, vsg::Object*)> onLinked)
    {
        for (auto& [ctrl, o] : clonedControllers)
            ctrl->link(ctx, *o);
        for (auto& [ctrl, o] : controllers)
            onLinked(ctrl, o);
        for (auto& [ctrl, o] : clonedControllers)
            onLinked(ctrl, o);
    }
}
