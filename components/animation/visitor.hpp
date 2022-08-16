#ifndef VSGOPENMW_ANIMATION_VISITOR_H
#define VSGOPENMW_ANIMATION_VISITOR_H

#include <vsg/core/Visitor.h>

#include "controller.hpp"

namespace Anim
{
    class Tags;

    /*
     * Extends vsg::Visitor with attachable animation objects.
     */
    class Visitor : public vsg::Visitor
    {
    public:
        virtual void apply(Controller &ctrl, vsg::Object &target) {}
        virtual void apply(const Tags&) {}

        using vsg::Visitor::apply;
        void apply(vsg::Object&) override;
        void apply(vsg::StateGroup&) override;
    };
}

#endif

