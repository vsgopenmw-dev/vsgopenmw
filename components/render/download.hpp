#ifndef VSGOPENMW_RENDER_DOWNLOAD_H
#define VSGOPENMW_RENDER_DOWNLOAD_H

#include <vsg/vk/Context.h>

namespace Render
{
    std::pair<vsg::ref_ptr<vsg::Commands>, vsg::ref_ptr<vsg::Image>> createDownloadCommands(vsg::ref_ptr<vsg::Device> device, vsg::ref_ptr<vsg::Image> srcImage, VkExtent2D extent, VkFormat srcImageFormat, VkImageLayout srcImageLayout);

    void submit(vsg::ref_ptr<vsg::Device> device, vsg::ref_ptr<vsg::Commands> commands);

    bool mapAndCopy(vsg::ref_ptr<vsg::Device> device, vsg::ref_ptr<vsg::Image> dstImage, vsg::ref_ptr<vsg::Data> out_data);

    vsg::ref_ptr<vsg::Data> download(vsg::ref_ptr<vsg::Device> device, vsg::ref_ptr<vsg::Image> srcImage, VkExtent2D extent, VkFormat srcImageFormat, VkImageLayout srcImageLayout);

    vsg::ref_ptr<vsg::Data> decompress(vsg::ref_ptr<vsg::Context> context, vsg::ref_ptr<vsg::Data> in_data);

    const auto downloadFormat = VK_FORMAT_R8G8B8A8_UNORM;
}

#endif
