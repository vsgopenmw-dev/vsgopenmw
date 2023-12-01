#ifndef VSGOPENMW_RENDER_RENDERTEXTURE_H
#define VSGOPENMW_RENDER_RENDERTEXTURE_H

#include <vsg/app/RenderGraph.h>

namespace Render // namespace vsgExamples
{
    std::pair<vsg::ref_ptr<vsg::RenderGraph>, vsg::ref_ptr<vsg::ImageView>> createRenderTexture(vsg::Context& context, VkSampleCountFlagBits samples, const VkExtent2D& extent,
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        VkImageLayout finalColorLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

#endif
