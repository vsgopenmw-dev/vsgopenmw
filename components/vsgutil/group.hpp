#ifndef VSGOPENMW_VSGUTIL_GROUP_H
#define VSGOPENMW_VSGUTIL_GROUP_H

#include <vsg/nodes/Group.h>

/*
 * Operates on group nodes.
 */
namespace vsgUtil
{
    // Moves children list into a new group.
    template <class Group, class Children, typename... Args>
    inline vsg::ref_ptr<vsg::Node> moveChildren(const Children& children, Args&&... args)
    {
        auto group = Group::create(std::forward<Args&&>(args)...);
        std::copy(children.begin(), children.end(), std::back_inserter(group->children));
        return group;
    }

    // Obtains a node containing specified children.
    template <class Children>
    inline vsg::ref_ptr<vsg::Node> getNode(const Children& children)
    {
        if (children.size() == 1)
            return children[0];
        return moveChildren<vsg::Group>(children);
    }

    /// Unpacks group.
    template <class Children>
    inline void removeGroup(Children& nodes)
    {
        if (nodes.size() == 1 && typeid(*nodes[0]) == typeid(vsg::Group) && !nodes[0]->getAuxiliary())
        {
            vsg::ref_ptr<vsg::Group> group{ static_cast<vsg::Group*>(nodes[0].get()) };
            nodes.clear();
            std::copy(group->children.begin(), group->children.end(), std::back_inserter(nodes));
        }
    }
}

#endif
