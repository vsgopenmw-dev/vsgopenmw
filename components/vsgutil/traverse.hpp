#ifndef VSGOPENMW_VSGUTIL_TRAVERSE_H
#define VSGOPENMW_VSGUTIL_TRAVERSE_H

#include <vsg/core/Visitor.h>
#include <vsg/core/ConstVisitor.h>

namespace vsgUtil
{
    /*
     * Recursively traverses objects supported by vsg::Visitor.
     */
    template<class Obj>
    class TTraverse : public vsg::Visitor
    {
    public:
        void apply(Obj &o) override
        {
            o.traverse(*this);
        }
    };
    using Traverse = TTraverse<vsg::Object>;

    /*
     * Recursively traverses const objects supported by vsg::ConstVisitor.
     */
    template<class Obj>
    class TConstTraverse : public vsg::ConstVisitor
    {
    public:
        void apply(const Obj &o) override
        {
            o.traverse(*this);
        }
    };
    using ConstTraverse = TConstTraverse<vsg::Object>;
}

#endif
