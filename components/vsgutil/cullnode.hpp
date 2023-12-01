#ifndef VSGOPENMW_VSGUTIL_CULLNODE_H
#define VSGOPENMW_VSGUTIL_CULLNODE_H

#include <vsg/nodes/Group.h>

namespace vsgUtil
{
    vsg::dsphere getBounds(vsg::ref_ptr<vsg::Node> node);

    vsg::ref_ptr<vsg::Node> createCullNode(vsg::ref_ptr<vsg::Node> node);

    // Moves children into CullNode or CullGroup.
    vsg::ref_ptr<vsg::Node> createCullNode(vsg::Group::Children& children);

    /*
     * Decorates StateGroups with a CullNode, if appropriate, and sets the subgraphRequiresLocalFrustum of any parent transforms to true.
     * The minCount parameter can be used to avoid adding a CullNode at the leaf level when there is only a single leaf.
     * In that case, it's more efficient to add a CullNode as the root node of the graph.
     */
    void addLeafCullNodes(vsg::ref_ptr<vsg::Node>& node, int minCount=0);
}

#endif
