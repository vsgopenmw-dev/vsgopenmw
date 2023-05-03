#ifndef VSGOPENMW_VSGUTIL_SHAREBUFFER_H
#define VSGOPENMW_VSGUTIL_SHAREBUFFER_H

#include <vsg/state/DescriptorBuffer.h>

#include <components/vsgutil/traversestate.hpp>

namespace vsgUtil
{
    /*
     * Reduces vsg::Buffer allocations.
     */
    class ShareBuffer : public vsgUtil::TraverseState
    {
        std::vector<vsg::BufferInfo*> mBufferInfos;
        std::set<vsg::BufferInfo*> mSeenBufferInfos;
        uint32_t mBufferUsage{};
    public:
        ShareBuffer() { overrideMask = vsg::MASK_ALL; }
        bool shareDynamicData = true;
        bool shareStaticData = false;

        using vsg::Visitor::apply;
        void apply(vsg::DescriptorBuffer& db)
        {
            for (auto& bi : db.bufferInfoList)
            {
                if (bi->buffer || !bi->data)
                    continue;
                bool dynamic = bi->data->dynamic();
                if ((dynamic && !shareDynamicData) || (!dynamic && !shareStaticData))
                    continue;
                if (mSeenBufferInfos.insert(bi.get()).second)
                {
                    mBufferInfos.push_back(bi.get());
                    if (dynamic)
                        mBufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                }
            }

            switch (db.descriptorType)
            {
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    mBufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                default:
                    mBufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                    break;
            }
        }
        void share(vsg::Device& device)
        {
            if (mBufferInfos.empty())
                return;

            // compute the total size of BufferInfo that needs to be allocated.
            VkDeviceSize alignment = 4;
            auto& limits = device.getPhysicalDevice()->getProperties().limits;
            if (mBufferUsage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
                alignment = std::max(alignment, limits.minStorageBufferOffsetAlignment);
            if (mBufferUsage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
                alignment = std::max(alignment, limits.minUniformBufferOffsetAlignment);

            VkDeviceSize totalSize = 0;
            for (auto& bufferInfo : mBufferInfos)
                totalSize += padBufferSize(bufferInfo->data->dataSize(), alignment);

            // if required allocate the buffer and reserve slots in it for the BufferInfo
            if (totalSize > 0)
            {
                auto buffer = vsg::Buffer::create(totalSize, mBufferUsage, VK_SHARING_MODE_EXCLUSIVE);

                for (auto& bufferInfo : mBufferInfos)
                {
                    auto size = padBufferSize(bufferInfo->data->dataSize(), alignment);
                    auto [allocated, offset] = buffer->reserve(size, alignment);
                    if (allocated)
                    {
                        bufferInfo->buffer = buffer;
                        bufferInfo->offset = offset;
                        bufferInfo->range = bufferInfo->data->dataSize();
                    }
                }
            }
        }
        size_t padBufferSize(size_t originalSize, VkDeviceSize alignment)
        {
            size_t alignedSize = originalSize;
            if (alignment > 0)
                alignedSize = (alignedSize + alignment - 1) & ~(alignment - 1);
            return alignedSize;
        }
    };
}

#endif
