#ifndef VSGOPENMW_VSGUTIL_SEARCHBYNAME_H
#define VSGOPENMW_VSGUTIL_SEARCHBYNAME_H

#include <vsg/core/Visitor.h>
#include <vsg/nodes/Node.h>

#include "name.hpp"

namespace vsgUtil
{
    /*
     * Searches for a node by name.
     */
    class SearchByName : public vsg::Visitor
    {
    public:
        std::string needle;
        vsg::Node *found = nullptr;
        using Compare = std::function<bool(const std::string &, const std::string &)>;
        Compare cmp;
        void apply(vsg::Node &node) override
        {
            auto name = getName(node);
            if (cmp(name, needle))
                found = &node;
            else
                node.traverse(*this);
        }
        static vsg::Node *search(vsg::Object &o, const std::string &name, Compare cmp)
        {
            SearchByName visitor;
            visitor.needle = name;
            visitor.cmp = cmp;
            o.accept(visitor);
            return visitor.found;
        }
    };
}

#endif
