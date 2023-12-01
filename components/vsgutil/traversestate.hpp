#ifndef VSGOPENMW_VSGUTIL_TRAVERSESTATE_H
#define VSGOPENMW_VSGUTIL_TRAVERSESTATE_H

#include "traverse.hpp"

namespace vsgUtil
{
    /*
     * Adds support for traversing StateGroup's stateCommands to vsg::Visitor.
     */
    class TraverseState : public vsgUtil::Traverse
    {
    public:
        using vsg::Visitor::apply;
        void apply(vsg::StateGroup& sg) override;
    };
}

#endif
