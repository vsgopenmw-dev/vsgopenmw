#ifndef VSGOPENMW_VSGUTIL_COMPOSITE_H
#define VSGOPENMW_VSGUTIL_COMPOSITE_H

#include <vsg/nodes/Node.h>

namespace vsgUtil
{
    /*
     * Basically composites node.
     */
    template <class Node>
    class Composite
    {
    protected:
        vsg::ref_ptr<Node> mNode;
    public:
        vsg::ref_ptr<vsg::Node> node() { return mNode; }
    };
}

#endif
