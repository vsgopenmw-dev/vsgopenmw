#ifndef VSGOPENMW_VIEW_COLLECTLIGHTS_H
#define VSGOPENMW_VIEW_COLLECTLIGHTS_H

#include <vsg/state/ViewDependentState.h>

namespace View
{
    class CollectLights : public vsg::ViewDependentState
    {
        vsg::ref_ptr<vsg::BufferedDescriptorBuffer> mDescriptor;
        vsg::ref_ptr<vsg::vec4Array> mLightData;
    public:
        CollectLights(vsg::ref_ptr<vsg::BufferedDescriptorBuffer> descriptor, vsg::ref_ptr<vsg::vec4Array> data);

        void advanceFrame() { mDescriptor->advanceFrame(); }
        void pack() override;
        void copy() override;
    };
}

#endif
