#ifndef VSGOPENMW_RENDER_MULTISAMPLE_H
#define VSGOPENMW_RENDER_MULTISAMPLE_H

#include <vsg/vk/PhysicalDevice.h>

namespace Render
{
    inline VkSampleCountFlagBits supportedSamples(VkSampleCountFlags requestedSamples, vsg::PhysicalDevice* physicalDevice)
    {
        if (requestedSamples != VK_SAMPLE_COUNT_1_BIT)
        {
            VkSampleCountFlags deviceColorSamples = physicalDevice->getProperties().limits.framebufferColorSampleCounts;
            VkSampleCountFlags deviceDepthSamples = physicalDevice->getProperties().limits.framebufferDepthSampleCounts;
            VkSampleCountFlags satisfied = deviceColorSamples & deviceDepthSamples & requestedSamples;
            if (satisfied != 0)
            {
                uint32_t highest = 1 << static_cast<uint32_t>(floor(log2(satisfied)));
                return static_cast<VkSampleCountFlagBits>(highest);
            }
        }
        return VK_SAMPLE_COUNT_1_BIT;
    }
}

#endif
