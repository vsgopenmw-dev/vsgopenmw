#ifndef VSGOPENMW_ANIMATION_VISITOR_H
#define VSGOPENMW_ANIMATION_VISITOR_H

#include <components/vsgutil/traversestate.hpp>

#include "controller.hpp"

namespace Anim
{
    class Tags;

    /*
     * Extends vsg::Visitor with attachable animation objects.
     */
    class Visitor : public vsgUtil::TraverseState
    {
    public:
        virtual void apply(Controller& ctrl, vsg::Object& target) {}
        virtual void apply(const Tags&) {}

        using vsg::Visitor::apply;
        void apply(vsg::Object&) override;
        void apply(vsg::DescriptorBuffer&) override;
    };
}

#endif
