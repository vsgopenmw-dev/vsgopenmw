#ifndef VSGOPENMW_ANIMATION_CLONE_H
#define VSGOPENMW_ANIMATION_CLONE_H

#include <vsg/nodes/Node.h>

#include "controllers.hpp"

namespace Anim
{
    struct Context;

    /*
     * Lists encountered animations.
     */
    struct CloneResult
    {
        Controllers controllers;
        std::vector<std::pair<Controller*, vsg::Object*>> clonedControllers; //{.updateOrder=controllers.updateOrder+1}
        template <class F>
        void accept(F func)
        {
            for (auto &[ctrl,o] : controllers)
                func(ctrl,o);
            for (auto &[ctrl,o] : clonedControllers)
                func(ctrl,o);
        }
        void link(Anim::Context &ctx);
    };

    /*
     * Overrides cloning logic by returning non-null object.
     */
    using CloneCallback = std::function<vsg::ref_ptr<vsg::Node>(const vsg::Node&)>;

    /*
     * Recursively clones a cached node as deeply as required for unique animation state.
     */
    CloneResult cloneIfRequired(vsg::ref_ptr<vsg::Node> &node, CloneCallback callback = {});
}

#endif
