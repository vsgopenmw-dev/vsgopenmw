#ifndef VSGOPENMW_RENDER_DOWNLOAD_H
#define VSGOPENMW_RENDER_DOWNLOAD_H

#include <optional>

#include <vsg/vk/Context.h>

namespace Render
{
    std::pair<vsg::ref_ptr<vsg::Commands>, vsg::ref_ptr<vsg::Image>> createDownloadCommands(
        vsg::Device* device, vsg::ref_ptr<vsg::Image> srcImage, VkRect2D srcRect, VkFormat srcImageFormat,
        VkImageLayout srcImageLayout, std::optional<VkExtent2D> dstExtent = {});

    void submit(vsg::Device* device, vsg::ref_ptr<vsg::Commands> commands);

    bool mapAndCopy(
        vsg::Device* device, vsg::ref_ptr<vsg::Image> dstImage, vsg::ref_ptr<vsg::Data> out_data);

    vsg::ref_ptr<vsg::Data> download(vsg::Device* device, vsg::ref_ptr<vsg::Image> srcImage,
        VkRect2D srcRect, VkFormat srcImageFormat, VkImageLayout srcImageLayout);
}

#endif
