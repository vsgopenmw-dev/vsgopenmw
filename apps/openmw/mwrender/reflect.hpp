#ifndef VSGOPENMW_MWRENDER_REFLECT_H
#define VSGOPENMW_MWRENDER_REFLECT_H

#include <vsg/commands/BlitImage.h>
#include <vsg/commands/Commands.h>
#include <vsg/commands/PipelineBarrier.h>
#include <vsg/state/DescriptorImage.h>

#include <components/pipeline/viewbindings.hpp>
#include <components/render/attachmentformat.hpp>
#include <components/render/engine.hpp>
#include <components/vsgutil/composite.hpp>
#include <components/vsgutil/image.hpp>

namespace MWRender
{
    /*
     * Stores rendered framebuffer.
     */
    class Reflect : public vsgUtil::Composite<vsg::Node>
    {
        Render::Engine& mEngine;
        vsg::ref_ptr<vsg::Image> mDstColorImage;
        vsg::ref_ptr<vsg::Image> mDstDepthImage;
        vsg::ref_ptr<vsg::DescriptorImage> mReflectColor;
        vsg::ref_ptr<vsg::DescriptorImage> mReflectDepth;
        vsg::ref_ptr<vsg::PipelineBarrier> mTransitionForTransferBarrier;
        vsg::ref_ptr<vsg::PipelineBarrier> mTransitionFromTransferBarrier;
        vsg::ref_ptr<vsg::BlitImage> mBlitColor;
        vsg::ref_ptr<vsg::BlitImage> mBlitDepth;

    public:
        Reflect(Render::Engine& engine)
            : mEngine(engine)
        {
            VkExtent2D res{ 512, 512 };
            mDstColorImage = vsgUtil::createImage(Render::compatibleColorFormat, res, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

            mDstDepthImage = vsgUtil::createImage(Render::compatibleDepthFormat, res, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

            auto initialColorLayout = mEngine.getPresentLayout();
            auto initialDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            mTransitionForTransferBarrier
                = vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    vsg::ImageMemoryBarrier::create(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_QUEUE_FAMILY_IGNORED,
                        VK_QUEUE_FAMILY_IGNORED, mDstColorImage,
                        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }),
                    vsg::ImageMemoryBarrier::create(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_QUEUE_FAMILY_IGNORED,
                        VK_QUEUE_FAMILY_IGNORED, mDstDepthImage,
                        VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }),
                    vsg::ImageMemoryBarrier::create(0, 0, initialColorLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                        /*srcColorImage*/ vsg::ref_ptr<vsg::Image>(),
                        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }),
                    vsg::ImageMemoryBarrier::create(0, 0, initialDepthLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                        /*srcDepthImage*/ vsg::ref_ptr<vsg::Image>(),
                        VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }));

            mTransitionFromTransferBarrier
                = vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    vsg::ImageMemoryBarrier::create(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, mDstColorImage,
                        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }),
                    vsg::ImageMemoryBarrier::create(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, mDstDepthImage,
                        VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }),
                    vsg::ImageMemoryBarrier::create(0, 0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, initialColorLayout,
                        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                        /*srcColorImage*/ vsg::ref_ptr<vsg::Image>(),
                        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }),
                    vsg::ImageMemoryBarrier::create(0, 0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, initialDepthLayout,
                        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                        /*srcDepthImage*/ vsg::ref_ptr<vsg::Image>(),
                        VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }));

            mBlitColor = vsg::BlitImage::create();
            VkImageBlit region{};
            region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
            region.srcOffsets[0] = region.dstOffsets[0] = VkOffset3D{ 0, 0, 0 };
            // region.srcOffsets[1] =
            region.dstOffsets[1] = VkOffset3D{ static_cast<int32_t>(res.width), static_cast<int32_t>(res.height), 1 };
            // mBlitColor->srcImage =
            mBlitColor->dstImage = mDstColorImage;
            mBlitColor->regions = { region };
            mBlitColor->filter = VK_FILTER_LINEAR;
            mBlitColor->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            mBlitColor->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            mBlitDepth = vsg::BlitImage::create();
            region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            mBlitDepth->dstImage = mDstDepthImage;
            mBlitDepth->regions = { region };
            mBlitDepth->filter = VK_FILTER_NEAREST;
            mBlitDepth->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            mBlitDepth->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            auto commands = vsg::Commands::create();
            commands->children
                = { mTransitionForTransferBarrier, mBlitColor, mBlitDepth, mTransitionFromTransferBarrier };
            mNode = commands;

            auto colorImageView = vsg::ImageView::create(mDstColorImage);
            mReflectColor = vsg::DescriptorImage::create(vsg::ImageInfo::create(vsg::Sampler::create(), colorImageView,
                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                Pipeline::Descriptors::VIEW_REFLECT_MAP_BINDING, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

            auto depthImageView = vsg::ImageView::create(mDstDepthImage);
            auto depthSampler = vsg::Sampler::create();
            depthSampler->minFilter = depthSampler->magFilter = VK_FILTER_NEAREST;
            mReflectDepth = vsg::DescriptorImage::create(
                vsg::ImageInfo::create(depthSampler, depthImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                Pipeline::Descriptors::VIEW_REFLECT_DEPTH_MAP_BINDING, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }
        vsg::ref_ptr<vsg::DescriptorImage> reflectColor() { return mReflectColor; }
        vsg::ref_ptr<vsg::DescriptorImage> reflectDepth() { return mReflectDepth; }
        void update()
        {
            auto srcColor = mEngine.colorImageView(0)->image;
            mTransitionForTransferBarrier->imageMemoryBarriers[2]->image
                = mTransitionFromTransferBarrier->imageMemoryBarriers[2]->image = mBlitColor->srcImage = srcColor;
            mBlitColor->regions[0].srcOffsets[1] = VkOffset3D{ static_cast<int32_t>(srcColor->extent.width),
                static_cast<int32_t>(srcColor->extent.height), 1 };

            auto srcDepth = mEngine.depthImageView()->image;
            mTransitionForTransferBarrier->imageMemoryBarriers[3]->image
                = mTransitionFromTransferBarrier->imageMemoryBarriers[3]->image = mBlitDepth->srcImage = srcDepth;
            mBlitDepth->regions[0].srcOffsets[1] = VkOffset3D{ static_cast<int32_t>(srcDepth->extent.width),
                static_cast<int32_t>(srcDepth->extent.height), 1 };
        }
    };
}

#endif
