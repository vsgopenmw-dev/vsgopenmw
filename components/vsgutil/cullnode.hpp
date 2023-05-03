#ifndef VSGOPENMW_VSGUTIL_CULLNODE_H
#define VSGOPENMW_VSGUTIL_CULLNODE_H

#include <vsg/nodes/Group.h>

namespace vsgUtil
{
    vsg::dsphere getBounds(vsg::ref_ptr<vsg::Node> node);

    vsg::ref_ptr<vsg::Node> createCullNode(vsg::ref_ptr<vsg::Node> node);

    // Moves children into CullNode or CullGroup.
    vsg::ref_ptr<vsg::Node> createCullNode(vsg::Group::Children& children);

    void addLeafCullNodes(vsg::ref_ptr<vsg::Node>& node, int minCount=0); //addLeafCullNodesAndSetSubgraphRequiresLocalFrustum
}

#endif
