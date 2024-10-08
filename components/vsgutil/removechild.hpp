#ifndef VSGOPENMW_VSGUTIL_REMOVECHILD_H
#define VSGOPENMW_VSGUTIL_REMOVECHILD_H

#include <vsg/core/ref_ptr.h>

#include <algorithm>

/*
 * Supports various types of groups.
 */
namespace vsgUtil
{
    template <class Group, class Child>
    void removeChild(Group parent, Child child)
    {
        auto found = std::find_if(parent->children.begin(), parent->children.end(),
            [child](const auto& rhs) { return &*child == &*rhs; });
        if (found != parent->children.end())
            parent->children.erase(found);
    }

    template <class Group, class Child>
    void removeSwitchChild(Group parent, Child child)
    {
        auto found = std::find_if(parent->children.begin(), parent->children.end(),
            [child](const auto& rhs) { return rhs.node == &*child; });
        if (found != parent->children.end())
            parent->children.erase(found);
    }

    template <class Group, class Node>
    void prune(const std::vector<Node>& path, Group* root)
    {
        for (auto it = path.rbegin(); it != path.rend(); ++it)
        {
            auto pit = it + 1;
            if (pit == path.rend())
            {
                removeChild(root, *it);
                break;
            }
            auto parent = static_cast<Group*>(&**pit);
            removeChild(parent, *it);
            if (!parent->children.empty())
                break;
        }
    }
}

#endif
