#include "computeimage.hpp"

#include <vsg/vk/State.h>
#include <vsg/commands/PipelineBarrier.h>

namespace Render
{
    ComputeImage::ComputeImage()
    {
        mDispatch = vsg::Dispatch::create(0, 0, 1);

        VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        mPreComputeBarrier = vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
            vsg::ImageMemoryBarrier::create(
                VK_ACCESS_TRANSFER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                vsg::ref_ptr<vsg::Image>(), range));
        mPostComputeBarrier = vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            vsg::ImageMemoryBarrier::create(
                0, 0, //View::computeBarrier()
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                vsg::ref_ptr<vsg::Image>(), range));

        mPreClearBarrier = vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            vsg::ImageMemoryBarrier::create(
                0, 0,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                vsg::ref_ptr<vsg::Image>(), range));
        mPostClearBarrier = vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            vsg::ImageMemoryBarrier::create(
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                vsg::ref_ptr<vsg::Image>(), range));
    }

    ComputeImage::~ComputeImage()
    {
    }

    void ComputeImage::disableDefaultPushConstants(vsg::RecordTraversal &visitor) const
    {
        visitor.getState()->projectionMatrixStack.dirty = false; // disable_pc_modelviewProjection
        visitor.getState()->modelviewMatrixStack.dirty = false;
    }

    void ComputeImage::clear(vsg::ref_ptr<vsg::Image> image, vsg::RecordTraversal &visitor) const
    {
        mPreClearBarrier->imageMemoryBarriers[0]->image = image;
        visitor.apply(*mPreClearBarrier);

        auto &commandBuffer = visitor.getState()->_commandBuffer;
        vkCmdClearColorImage(*commandBuffer, image->vk(commandBuffer->deviceID), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &mClearColor, static_cast<uint32_t>(1), &mPreClearBarrier->imageMemoryBarriers[0]->subresourceRange);

        mPostClearBarrier->imageMemoryBarriers[0]->image = image;
        visitor.apply(*mPostClearBarrier);
    }

    void ComputeImage::handleClearRequests(vsg::RecordTraversal &visitor) const
    {
        for (auto &r : clearRequests)
            clear(r.image, visitor);
        clearRequests.clear();
    }

    void ComputeImage::compute(vsg::RecordTraversal &visitor, vsg::ref_ptr<vsg::Image> image, uint32_t groupCountX, uint32_t groupCountY) const
    {
        mPreComputeBarrier->imageMemoryBarriers[0]->image = image;
        visitor.apply(*mPreComputeBarrier);

        mDispatch->groupCountX = groupCountX;
        mDispatch->groupCountY = groupCountY;

        traverse(visitor);

        mPostComputeBarrier->imageMemoryBarriers[0]->image = image;
        visitor.apply(*mPostComputeBarrier);
    }
}
