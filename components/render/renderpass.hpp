#ifndef VSGOPENMW_RENDER_RENDERPASS_H
#define VSGOPENMW_RENDER_RENDERPASS_H

#include <vsg/vk/RenderPass.h>

namespace Render
{
    inline vsg::RenderPass::Dependencies standardDependencies()
    {
        // image layout transition
        // depth buffer is shared between swap chain images
        // color buffer can be written by a previous render pass
        vsg::SubpassDependency dependency = {
            VK_SUBPASS_EXTERNAL, 0,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
        };
        return { dependency };
    }

    /*
     * Creates general purpose compatible render pass.
     */
    inline vsg::ref_ptr<vsg::RenderPass> createStandardRenderPass(vsg::Device* device, VkFormat imageFormat, VkFormat depthFormat, bool load, bool store, VkImageLayout finalColorLayout)
    {
        auto colorAttachment = vsg::defaultColorAttachment(imageFormat);
        auto depthAttachment = vsg::defaultDepthAttachment(depthFormat);
        colorAttachment.finalLayout = finalColorLayout;
        if (load)
        {
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.initialLayout = colorAttachment.finalLayout;
            depthAttachment.initialLayout = depthAttachment.finalLayout;
        }
        if (store)
        {
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }

        vsg::RenderPass::Attachments attachments{colorAttachment, depthAttachment};

        vsg::AttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        vsg::AttachmentReference depthAttachmentRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        vsg::SubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachments.emplace_back(colorAttachmentRef);
        subpass.depthStencilAttachments.emplace_back(depthAttachmentRef);

        vsg::RenderPass::Subpasses subpasses{subpass};

        return vsg::RenderPass::create(device, attachments, subpasses, standardDependencies());
    }

    /*
     * Creates general purpose compatible multisampled render pass.
     */
    inline vsg::ref_ptr<vsg::RenderPass> createMultisampledRenderPass(vsg::Device* device, VkFormat imageFormat, VkFormat depthFormat, VkSampleCountFlagBits samples, bool load, bool store, VkImageLayout finalColorLayout)
    {
        if (samples == VK_SAMPLE_COUNT_1_BIT)
        {
            return createStandardRenderPass(device, imageFormat, depthFormat, load, store, finalColorLayout);
        }

        // First attachment is multisampled target.
        vsg::AttachmentDescription colorAttachment = {};
        colorAttachment.format = imageFormat;
        colorAttachment.samples = samples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if (load)
        {
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.initialLayout = colorAttachment.finalLayout;
        }
        if (store)
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Second attachment is the resolved image which will be presented.
        vsg::AttachmentDescription resolveAttachment = {};
        resolveAttachment.format = imageFormat;
        resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolveAttachment.finalLayout = finalColorLayout;

        // Multisampled depth attachment. It usually won't be resolved.
        vsg::AttachmentDescription depthAttachment = {};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = samples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        if (load)
        {
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment.initialLayout = depthAttachment.finalLayout;
        }
        if (store)
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        vsg::RenderPass::Attachments attachments{colorAttachment, resolveAttachment, depthAttachment};

        //if (store || ensureCompatibleRenderPass)
        {
            vsg::AttachmentDescription depthResolveAttachment = {};
            depthResolveAttachment.format = depthFormat;
            depthResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments.push_back(depthResolveAttachment);
        }

        vsg::AttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        vsg::AttachmentReference resolveAttachmentRef = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        vsg::AttachmentReference depthAttachmentRef = { 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        vsg::SubpassDescription subpass;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachments.emplace_back(colorAttachmentRef);
        subpass.resolveAttachments.emplace_back(resolveAttachmentRef);
        subpass.depthStencilAttachments.emplace_back(depthAttachmentRef);

        // "As an additional special case, if two render passes have a single subpass, the resolve attachment reference compatibility requirements are ignored."
        // if (store || ensureCompatible)
        if (store)
        {
            vsg::AttachmentReference depthResolveAttachmentRef = {};
            depthResolveAttachmentRef.attachment = 3;
            depthResolveAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            //if (supportsAverage) subpass.depthResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
            subpass.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
            subpass.stencilResolveMode = VK_RESOLVE_MODE_NONE;
            subpass.depthStencilResolveAttachments.emplace_back(depthResolveAttachmentRef);
        }

        vsg::RenderPass::Subpasses subpasses{subpass};

        return vsg::RenderPass::create(device, attachments, subpasses, standardDependencies());
    }
}

#endif
