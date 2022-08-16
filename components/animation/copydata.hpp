#ifndef VSGOPENMW_ANIMATION_COPYDATA_H
#define VSGOPENMW_ANIMATION_COPYDATA_H

#include "tcontroller.hpp"
#include "tmutable.hpp"

#include <vsg/state/BufferedDescriptorBuffer.h>

namespace Anim
{
    /*
     * Writes data updated by const Derived into Descriptor.
     */
    template <class Derived, class DataType=vsg::Data>
    class CopyData : public TController<CopyData<Derived, DataType>, vsg::BufferedDescriptorBuffer>
    {
    public:
        void apply(vsg::BufferedDescriptorBuffer &descriptor, float time) const
        {
            /*if (*/static_cast<const Derived&>(*this).apply(static_cast<DataType&>(*descriptor.bufferInfoList[0]->data), time);
            descriptor.copyDataListToBuffers();
        }
    };

    /*
     * Writes data statefully updated by Derived into Descriptor.
     */
    template <class Derived, class DataType=vsg::Data>
    class MCopyData : public TMutable<MCopyData<Derived, DataType>, vsg::BufferedDescriptorBuffer>
    {
    public:
        void apply(vsg::BufferedDescriptorBuffer &descriptor, float time)
        {
            /*if (*/static_cast<Derived&>(*this).apply(static_cast<DataType&>(*descriptor.bufferInfoList[0]->data), time);
            descriptor.copyDataListToBuffers();
        }
        vsg::ref_ptr<Controller> cloneIfRequired() const override { return vsg::ref_ptr{new Derived(static_cast<const Derived&>(*this))}; }
    };
}

#endif
