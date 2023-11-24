#ifndef VSGOPENMW_VIEW_COLLECTLIGHTS_H
#define VSGOPENMW_VIEW_COLLECTLIGHTS_H

#include <vsg/state/ViewDependentState.h>

namespace View
{
    class CollectLights : public vsg::ViewDependentState
    {
        vsg::ref_ptr<vsg::vec4Array> mData;

    public:
        CollectLights(size_t maxLights);

        vsg::ref_ptr<vsg::vec4Array> data() { return mData; }

        void traverse(vsg::Visitor& visitor) override {}
        void traverse(vsg::ConstVisitor& visitor) const override {}

        void compile(vsg::Context&) override {}
        void pack();
    };
}

#endif
