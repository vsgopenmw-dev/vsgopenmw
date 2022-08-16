#ifndef VSGOPENMW_PIPELINE_DESCRIPTORVALUE_H
#define VSGOPENMW_PIPELINE_DESCRIPTORVALUE_H

#include <vsg/state/BufferedDescriptorBuffer.h>

namespace Pipeline
{
    /*
     * Conveniently manages a descriptor value for updating.
     */
    template <class Data>
    class DynamicDescriptorValue
    {
   public:
        using value_type = vsg::Value<Data>;
        DynamicDescriptorValue(int binding)
        {
            mValue = value_type::create();
            mDescriptor = vsg::BufferedDescriptorBuffer::create(mValue, binding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        }

        vsg::ref_ptr<vsg::BufferedDescriptorBuffer> descriptor() { return mDescriptor; }
        Data &value() { return mValue->value(); }
        void copyDataListToBuffers()
        {
            mDescriptor->copyDataListToBuffers();
        }
    private:
        vsg::ref_ptr<value_type> mValue;
        vsg::ref_ptr<vsg::BufferedDescriptorBuffer> mDescriptor;
    };

    /*
     * Conveniently manages a descriptor value.
     */
    template <class Data>
    class DescriptorValue
    {
   public:
        using value_type = vsg::Value<Data>;
        DescriptorValue(int binding)
        {
            mValue = value_type::create();
            mDescriptor = vsg::DescriptorBuffer::create(mValue, binding);
        }

        vsg::ref_ptr<vsg::DescriptorBuffer> descriptor() { return mDescriptor; }
        Data &value() { return mValue->value(); }
    private:
        vsg::ref_ptr<value_type> mValue;
        vsg::ref_ptr<vsg::DescriptorBuffer> mDescriptor;
    };
}

#endif
