#ifndef VSGOPENMW_VSGUTIL_ADDCHILDREN_H
#define VSGOPENMW_VSGUTIL_ADDCHILDREN_H

#include <vsg/nodes/Group.h>

namespace vsgUtil
{
    /*
     * Attempts to remove redundant nodes.
     */
    inline void addChildren(vsg::Group &parent, vsg::Node &children)
    {
        if (typeid(children) == typeid(vsg::Group))
        {
            auto &childrenList = static_cast<vsg::Group&>(children).children;
            for (auto &child : childrenList)
            {
                if (typeid(*child) != typeid(vsg::Node))
                    parent.children.emplace_back(child);
            }
        }
        else
            parent.children.emplace_back(&children);
    }
}

#endif
