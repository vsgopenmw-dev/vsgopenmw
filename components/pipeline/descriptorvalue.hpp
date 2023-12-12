#ifndef VSGOPENMW_PIPELINE_DESCRIPTORVALUE_H
#define VSGOPENMW_PIPELINE_DESCRIPTORVALUE_H

#include <vsg/state/DescriptorBuffer.h>

namespace Pipeline
{
    /*
     * Conveniently manages a descriptor's value for updating.
     */
    template <class Data>
    class DynamicDescriptorValue
    {
    public:
        using value_type = vsg::Value<Data>;
        DynamicDescriptorValue(
            int binding, int dstArrayElement = 0, VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            mValue = value_type::create();
            mValue->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;
            mDescriptor = vsg::DescriptorBuffer::create(mValue, binding, dstArrayElement, descriptorType);
        }

        template <class Property>
        void update(Property& p, const Property& newValue)
        {
            if (p != newValue)
            {
                p = newValue;
                dirty();
            }
        }

        vsg::ref_ptr<vsg::DescriptorBuffer> descriptor() { return mDescriptor; }
        Data& value() { return mValue->value(); }
        void dirty() { mValue->dirty(); }

    private:
        vsg::ref_ptr<value_type> mValue;
        vsg::ref_ptr<vsg::DescriptorBuffer> mDescriptor;
    };

    /*
     * Conveniently manages a descriptor's value.
     */
    template <class Data>
    class DescriptorValue
    {
    public:
        using value_type = vsg::Value<Data>;
        DescriptorValue(
            int binding, int dstArrayElement = 0, VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            mValue = value_type::create();
            mDescriptor = vsg::DescriptorBuffer::create(mValue, binding, dstArrayElement, descriptorType);
        }

        vsg::ref_ptr<vsg::DescriptorBuffer> descriptor() { return mDescriptor; }
        Data& value() { return mValue->value(); }

    private:
        vsg::ref_ptr<value_type> mValue;
        vsg::ref_ptr<vsg::DescriptorBuffer> mDescriptor;
    };
}

#endif
