#include "download.hpp"

#include <iostream>

#include <vsg/commands/CopyAndReleaseImage.h>
#include <vsg/commands/PipelineBarrier.h>
#include <vsg/commands/BlitImage.h>
#include <vsg/commands/CopyImage.h>
#include <vsg/commands/Commands.h>
#include <vsg/vk/CommandPool.h>
#include <vsg/vk/SubmitCommands.h>

namespace Render //namespace vsgExamples
{
    std::pair<vsg::ref_ptr<vsg::Commands>, vsg::ref_ptr<vsg::Image>> createDownloadCommands(vsg::ref_ptr<vsg::Device> device, vsg::ref_ptr<vsg::Image> srcImage, VkExtent2D extent, VkFormat srcImageFormat, VkImageLayout srcImageLayout)
    {
        auto targetImageFormat = srcImageFormat;

        //
        // 1) Check to see of Blit is supported.
        //
        auto physicalDevice = device->getPhysicalDevice();
        VkFormatProperties srcFormatProperties;
        vkGetPhysicalDeviceFormatProperties(*(physicalDevice), srcImageFormat, &srcFormatProperties);

        VkFormatProperties destFormatProperties;
        vkGetPhysicalDeviceFormatProperties(*(physicalDevice), downloadFormat, &destFormatProperties);

        bool supportsBlit = ((srcFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0) &&
                            ((destFormatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0);
        if (supportsBlit)
        {
            // we can automatically convert the image format when blit, so take advantage of it to ensure RGBA
            targetImageFormat = downloadFormat;
        }

        //
        // 2) create image to write to
        //
        auto dstImage = vsg::Image::create();
        dstImage->format = targetImageFormat;
        dstImage->extent = {extent.width,extent.height,1};
        dstImage->arrayLayers = 1;
        dstImage->mipLevels = 1;
        dstImage->tiling = VK_IMAGE_TILING_LINEAR;
        dstImage->usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        dstImage->compile(device);

        auto deviceMemory = vsg::DeviceMemory::create(device, dstImage->getMemoryRequirements(device->deviceID), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        dstImage->bind(deviceMemory, 0);

        //
        // 3) create commands
        auto commands = vsg::Commands::create();

        // 3.a) transition dstImage to transfer destination initialLayout
        auto transitiondstImageToDestinationLayoutBarrier = vsg::ImageMemoryBarrier::create(
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            dstImage,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        );

        // 3.b) transition image from current to transfer source initialLayout
        auto transitionsrcImageToTransferSourceLayoutBarrier = vsg::ImageMemoryBarrier::create(
            VK_ACCESS_MEMORY_READ_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            srcImageLayout,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            srcImage,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        );

        auto cmd_transitionForTransferBarrier = vsg::PipelineBarrier::create(
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            transitiondstImageToDestinationLayoutBarrier,
            transitionsrcImageToTransferSourceLayoutBarrier
        );

        commands->addChild(cmd_transitionForTransferBarrier);

        if (supportsBlit)
        {
            // 3.c.1) if blit using VkCmdBlitImage
            VkImageBlit region{};
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.layerCount = 1;
            region.srcOffsets[0] = {0, 0, 0};
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.layerCount = 1;
            region.dstOffsets[0] = {0, 0, 0};

            region.srcOffsets[1] = region.dstOffsets[1] = {static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1};

            auto blitImage = vsg::BlitImage::create();
            blitImage->srcImage = srcImage;
            blitImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitImage->dstImage = dstImage;
            blitImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitImage->regions = {region};
            blitImage->filter = VK_FILTER_NEAREST; //;

            commands->addChild(blitImage);
        }
        else
        {
            // 3.c.2) else use VkCmdCopyImage
            VkImageCopy region{};
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.layerCount = 1;
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.layerCount = 1;
            region.extent = {extent.width,extent.height,1};

            auto copyImage = vsg::CopyImage::create();
            copyImage->srcImage = srcImage;
            copyImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            copyImage->dstImage = dstImage;
            copyImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            copyImage->regions = {region};

            commands->addChild(copyImage);
        }

        // 3.d) transition destinate image from transfer destination layout to general layout to enable mapping to image DeviceMemory
        auto transitiondstImageToMemoryReadBarrier = vsg::ImageMemoryBarrier::create(
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            dstImage,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        );

        // 3.e) transition source image back
        auto transitionsrcImageBackBarrier = vsg::ImageMemoryBarrier::create(
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            srcImageLayout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            srcImage,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        );

        auto cmd_transitionFromTransferBarrier = vsg::PipelineBarrier::create(
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            transitiondstImageToMemoryReadBarrier,
            transitionsrcImageBackBarrier
        );

        commands->addChild(cmd_transitionFromTransferBarrier);
        return {commands, dstImage};
    }

    bool mapAndCopy(vsg::ref_ptr<vsg::Device> device, vsg::ref_ptr<vsg::Image> dstImage, vsg::ref_ptr<vsg::Data> out_data)
    {
        if (out_data->getLayout().format != dstImage->format)
            return false;

        //
        // 4) map image and copy
        //
        VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout(*device, dstImage->vk(device->deviceID), &subResource, &subResourceLayout);

        auto deviceMemory = dstImage->getDeviceMemory(device->deviceID);

        // Map the buffer memory that will automatically unmap itself on destruction.
        auto mappedData = vsg::MappedData<vsg::ubyteArray>::create(deviceMemory, subResourceLayout.offset, 0, vsg::Data::Layout{.stride=1}, subResourceLayout.size);
        auto w = dstImage->extent.width;
        auto h = dstImage->extent.height;
        for (uint32_t row=0; row<h; ++row)
            std::memcpy(out_data->dataPointer(row*w), mappedData->dataPointer(row*subResourceLayout.rowPitch), w*out_data->getLayout().stride);
        return true;
    }

    vsg::ref_ptr<vsg::Data> download(vsg::ref_ptr<vsg::Device> device, vsg::ref_ptr<vsg::Image> srcImage, VkExtent2D extent, VkFormat srcImageFormat, VkImageLayout srcImageLayout)
    {
        auto [commands, dstImage] = createDownloadCommands(device, srcImage, extent, srcImageFormat, srcImageLayout);
        submit(device, commands);

        vsg::ref_ptr<vsg::Data> array;
        switch (dstImage->format)
        {
        case VK_FORMAT_R8G8B8A8_UNORM:
        {
            array = vsg::ubvec4Array2D::create(dstImage->extent.width, dstImage->extent.height, vsg::Data::Layout{dstImage->format});
            break;
        }
        case VK_FORMAT_R8G8B8_UNORM:
            array = vsg::ubvec3Array2D::create(dstImage->extent.width, dstImage->extent.height, vsg::Data::Layout{dstImage->format});
        default:
            std::cerr << "!download(VkFormat(" << dstImage->format << std::endl;
            return {};
        }

        mapAndCopy(device, dstImage, array);
        return array;
    }

    void submit(vsg::ref_ptr<vsg::Device> device, vsg::ref_ptr<vsg::Commands> commands)
    {
        auto physicalDevice = device->getPhysicalDevice();
        auto fence = vsg::Fence::create(device);
        auto queueFamilyIndex = physicalDevice->getQueueFamily(VK_QUEUE_GRAPHICS_BIT);
        auto commandPool = vsg::CommandPool::create(device, queueFamilyIndex);
        auto queue = device->getQueue(queueFamilyIndex);

        vsg::submitCommandsToQueue(commandPool, fence, 100000000000, queue, [&](vsg::CommandBuffer& commandBuffer) {
            commands->record(commandBuffer);
        });
    }

    vsg::ref_ptr<vsg::Data> decompress(vsg::ref_ptr<vsg::Context> context, vsg::ref_ptr<vsg::Data> in_data)
    {
        auto layout = in_data->getLayout();
        if (layout.blockDepth == 1 && layout.blockWidth == 1 && layout.blockHeight == 1)
            return in_data;

        auto device = context->device;
        auto srcImage = vsg::Image::create(in_data);
        srcImage->usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        srcImage->compile(device);

        auto deviceMemory = vsg::DeviceMemory::create(device, srcImage->getMemoryRequirements(device->deviceID), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        srcImage->bind(deviceMemory, 0);

        auto imageView = vsg::ImageView::create(srcImage);
        auto uploadImageCommand = vsg::CopyAndReleaseImage::create(context->stagingMemoryBufferPools);
        uploadImageCommand->copy(in_data, vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(), imageView, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));

        uint32_t w = srcImage->extent.width; //in_data->width()* blockWidth
        uint32_t h = srcImage->extent.height; //in_data->height()* blockHeight

        auto [commands, dstImage] = Render::createDownloadCommands(device, srcImage, {w,h}, srcImage->format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vsg::Commands::Children newChildren = {uploadImageCommand};
        std::copy(commands->children.begin(), commands->children.end(), std::back_inserter(newChildren));
        commands->children = newChildren;

        submit(device, commands);
        auto outData = vsg::ubvec4Array2D::create(w, h, vsg::Data::Layout{.format=VK_FORMAT_R8G8B8A8_UNORM});
        if (mapAndCopy(device, dstImage, outData))
            return outData;
        return {};
    }
}
