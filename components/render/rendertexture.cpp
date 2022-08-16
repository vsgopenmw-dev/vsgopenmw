#include "rendertexture.hpp"

#include <vsg/vk/Context.h>

#include "attachmentformat.hpp"

namespace Render
{
    vsg::ref_ptr<vsg::RenderGraph> createRenderTexture(vsg::Context &context, const VkExtent2D &extent, vsg::ref_ptr<vsg::ImageView> &colorImageView, int usage, VkImageLayout finalColorLayout)
    {
        VkExtent3D extent3d{extent.width, extent.height, 1};

        // Attachments
        // create image for color attachment
        auto colorImage = vsg::Image::create();
        colorImage->format = compatibleColorFormat;
        colorImage->extent = extent3d;
        colorImage->mipLevels = 1;
        colorImage->arrayLayers = 1;
        colorImage->usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | usage;
        colorImageView = vsg::createImageView(context, colorImage, VK_IMAGE_ASPECT_COLOR_BIT);

        // create depth buffer
        auto depthImage = vsg::Image::create();
        depthImage->extent = extent3d;
        depthImage->mipLevels = 1;
        depthImage->arrayLayers = 1;
        depthImage->format = compatibleDepthFormat;
        depthImage->usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        auto depthImageView = vsg::createImageView(context, depthImage, vsg::computeAspectFlagsForFormat(depthImage->format));

        // attachment descriptions
        vsg::RenderPass::Attachments attachments(2);

        // Color attachment
        attachments[0] = vsg::defaultColorAttachment(colorImage->format);
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = finalColorLayout;

        // Depth attachment
        attachments[1] = vsg::defaultDepthAttachment(depthImage->format);

        vsg::AttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        vsg::AttachmentReference depthReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        vsg::RenderPass::Subpasses subpassDescription(1);
        subpassDescription[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription[0].colorAttachments = {colorReference};
        subpassDescription[0].depthStencilAttachments = {depthReference};

        vsg::RenderPass::Dependencies dependencies(2);

        // XXX This dependency is copied from the offscreenrender.cpp
        // example. I don't completely understand it, but I think it's
        // purpose is to create a barrier if some earlier render pass was
        // using this framebuffer's attachment as a texture.
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // This is the heart of what makes Vulkan offscreen rendering
        // work: render passes that follow are blocked from using this
        // passes' color attachment in their fragment shaders until all
        // this pass' color writes are finished.
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        auto rendergraph = vsg::RenderGraph::create();
        rendergraph->renderArea.offset = VkOffset2D{0, 0};
        rendergraph->renderArea.extent = extent;
        rendergraph->framebuffer = vsg::Framebuffer::create(
            vsg::RenderPass::create(context.device, attachments, subpassDescription, dependencies),
            vsg::ImageViews{colorImageView, depthImageView},
            extent.width, extent.height, 1);
        return rendergraph;
    }
}
