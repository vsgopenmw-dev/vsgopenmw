#ifndef VSGOPENMW_ANIMATION_CLONE_H
#define VSGOPENMW_ANIMATION_CLONE_H

#include <vsg/nodes/Node.h>

#include "animation.hpp"

namespace Anim
{
    /*
     * Overrides cloning logic by returning non-null object.
     * If the cloneCallback returns a valid object then that object is used, otherwise the default cloning behavior is used.
     */
    using CloneCallback = std::function<vsg::ref_ptr<vsg::Node>(const vsg::Node&)>;

    /*
     * Recursively clones input scene graph as deeply as required for unique animation state.
     * Any node that has a controller attached to itself or to any of its children is cloned, otherwise shallow copying is used.
     * If any nodes have been cloned then the new hierarchy will be written to the &node parameter.
     * Cloning ensures that animations can be played on the clone without affecting the original scene graph that was cloned from, and is typically used to avoid loading the same file from disk multiple times when multiple instances of a model are used in the scene.
     * The cloneCallback parameter can be used to forbid shallow copies of particular objects, replace unwanted nodes with empty nodes or replace empty placeholder nodes with real nodes.
     */
    Animation cloneIfRequired(vsg::ref_ptr<vsg::Node>& node, CloneCallback callback = {});
}

#endif
