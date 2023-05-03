#ifndef VSGOPENMW_ANIMATION_CLONE_H
#define VSGOPENMW_ANIMATION_CLONE_H

#include <vsg/nodes/Node.h>

#include "animation.hpp"

namespace Anim
{
    /*
     * Overrides cloning logic by returning non-null object.
     */
    using CloneCallback = std::function<vsg::ref_ptr<vsg::Node>(const vsg::Node&)>;

    /*
     * Recursively clones a cached node as deeply as required for unique animation state.
     */
    Animation cloneIfRequired(vsg::ref_ptr<vsg::Node>& node, CloneCallback callback = {});
}

#endif
