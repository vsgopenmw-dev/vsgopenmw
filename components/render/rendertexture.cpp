#include "rendertexture.hpp"

#include <vsg/vk/Context.h>

#include <components/vsgutil/image.hpp>

#include "attachmentformat.hpp"
#include "renderpass.hpp"

namespace Render
{
    std::pair<vsg::ref_ptr<vsg::RenderGraph>, vsg::ref_ptr<vsg::ImageView>> createRenderTexture(vsg::Context& context, VkSampleCountFlagBits samples, const VkExtent2D& extent, VkImageUsageFlags usage, VkImageLayout finalColorLayout)
    {
        // Attachments
        // create image for color attachment
        auto colorImage = vsgUtil::createImage(compatibleColorFormat, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | usage);
        auto colorImageView = vsg::createImageView(context, colorImage, VK_IMAGE_ASPECT_COLOR_BIT);

        // create depth buffer
        auto depthImage = vsgUtil::createImage(compatibleDepthFormat, extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, samples);
        auto depthImageView
            = vsg::createImageView(context, depthImage, vsg::computeAspectFlagsForFormat(depthImage->format));

        auto renderPass = createMultisampledRenderPass(context.device, compatibleColorFormat, compatibleDepthFormat, samples, false, false, finalColorLayout);

        vsg::ImageViews attachments;
        if (samples == VK_SAMPLE_COUNT_1_BIT)
            attachments = { colorImageView, depthImageView };
        else
        {
            auto multisampleImage = vsgUtil::createImage(compatibleColorFormat, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, samples);
            auto multisampleImageView = vsg::createImageView(context, multisampleImage, VK_IMAGE_ASPECT_COLOR_BIT);

            /*
            auto depthResolveImage = vsgUtil::createImage(compatibleDepthFormat, extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            auto depthResolveImageView = vsg::createImageView(context, depthResolveImage, vsg::computeAspectFlagsForFormat(depthResolveImage->format));
            */
            attachments = { multisampleImageView, colorImageView, depthImageView, /*depthResolveImageView*/ depthImageView };
        }

        auto rendergraph = vsg::RenderGraph::create();
        rendergraph->renderArea.offset = { 0, 0 };
        rendergraph->renderArea.extent = extent;
        rendergraph->framebuffer = vsg::Framebuffer::create(
            renderPass, attachments, extent.width, extent.height, 1);
        return std::make_pair(rendergraph, colorImageView);
    }
}
