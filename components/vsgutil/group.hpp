#ifndef VSGOPENMW_VSGUTIL_GROUP_H
#define VSGOPENMW_VSGUTIL_GROUP_H

#include <vsg/nodes/Group.h>

namespace vsgUtil
{
    // Moves children list into a new group.
    template <class Group, typename ... Args>
    inline vsg::ref_ptr<vsg::Node> moveChildren(const vsg::Group::Children &children, Args&& ... args)
    {
        auto group = Group::create(std::forward<Args&&>(args)...);
        group->children = children;
        return group;
    }

    // Obtains a node containing specified children.
    inline vsg::ref_ptr<vsg::Node> getNode(vsg::Group::Children &children)
    {
        if (children.size()==1)
            return children[0];
        return moveChildren<vsg::Group>(children);
    }

    /// Unpacks group.
    inline void removeGroup(vsg::Group::Children &nodes)
    {
        if (nodes.size() == 1 && typeid(*nodes[0]) == typeid(vsg::Group) && !nodes[0]->getAuxiliary())
            nodes = static_cast<vsg::Group*>(nodes[0].get())->children;
    }
}

#endif
