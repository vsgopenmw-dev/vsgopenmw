#ifndef VSGOPENMW_VSGUTIL_SUSPENDRENDERPASS_H
#define VSGOPENMW_VSGUTIL_SUSPENDRENDERPASS_H

#include <cassert>

#include <vsg/app/RenderGraph.h>
#include <vsg/vk/CommandBuffer.h>

namespace vsgUtil
{
    /*
     * Contains commands that must not be executed in a render pass instance.
     */
    class SuspendRenderPass : public vsg::Group
    {
        vsg::RenderGraph* mParent{};
        vsg::ref_ptr<vsg::RenderPass> mRenderPass;

    public:
        SuspendRenderPass(const SuspendRenderPass& rhs)
            : mParent(rhs.mParent)
            , mRenderPass(rhs.mRenderPass)
        {
        }
        SuspendRenderPass(vsg::ref_ptr<vsg::RenderGraph> parent)
            : mParent(parent.get())
        {
            assert(parent->contents == VK_SUBPASS_CONTENTS_INLINE);

            auto baseRenderPass = parent->getRenderPass();
            auto newAttachments = baseRenderPass->attachments;
            for (auto& attachment : newAttachments)
            {
                assert(attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE);
                attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachment.initialLayout = attachment.finalLayout;
            }
            mRenderPass = vsg::RenderPass::create(baseRenderPass->device, newAttachments, baseRenderPass->subpasses,
                baseRenderPass->dependencies, baseRenderPass->correlatedViewMasks);
        }

        void accept(vsg::RecordTraversal& recordTraversal) const
        {
            VkCommandBuffer vk_commandBuffer = *(recordTraversal.getCommandBuffer());
            vkCmdEndRenderPass(vk_commandBuffer);

            traverse(recordTraversal);

            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

            if (auto& framebuffer = mParent->framebuffer)
                renderPassInfo.framebuffer = *(framebuffer);
            else
            {
                auto& window = mParent->window;
                size_t imageIndex = std::min(window->imageIndex(), window->numFrames() - 1);
                renderPassInfo.framebuffer = *(window->framebuffer(imageIndex));
            }

            renderPassInfo.renderPass = (*mRenderPass);
            renderPassInfo.renderArea = mParent->renderArea;
            renderPassInfo.clearValueCount = 0;
            renderPassInfo.pClearValues = nullptr;

            vkCmdBeginRenderPass(vk_commandBuffer, &renderPassInfo, mParent->contents);
        }
    };
}

#endif
