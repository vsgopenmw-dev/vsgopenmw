#ifndef VSGOPENMW_RENDER_ATTACHMENTFORMAT_H
#define VSGOPENMW_RENDER_ATTACHMENTFORMAT_H

#include <vsg/vk/vulkan.h>

namespace Render
{
    /*
     * Ensures render pass compatibility.
     * VUID-vkCmdDrawIndexed-renderPass-02684
     */
    const auto compatibleColorFormat = VK_FORMAT_B8G8R8A8_UNORM; // = SDL_GetWindowSurfaceFormat(
    const auto compatibleDepthFormat = VK_FORMAT_D32_SFLOAT;
    // const VkSampleCountFlagBits compatibleSamples = mEngine.framebufferSamples();
}

#endif
