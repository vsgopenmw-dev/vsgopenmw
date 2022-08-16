#ifndef VSGOPENMW_VSGUTIL_CULLNODE_H
#define VSGOPENMW_VSGUTIL_CULLNODE_H

#include <vsg/nodes/Group.h>

namespace vsgUtil
{
    // Adds CullNode.
    vsg::ref_ptr<vsg::Node> createCullNode(vsg::Node &node);

    // Moves children into CullNode or CullGroup.
    vsg::ref_ptr<vsg::Node> createCullNode(vsg::Group::Children &children);

    // Decorates StateGroups.
    void addLeafCullNodes(vsg::Node &n);
}

#endif
