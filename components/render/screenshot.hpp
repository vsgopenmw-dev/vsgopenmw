#ifndef VSGOPENMW_RENDER_SCREENSHOT_H
#define VSGOPENMW_RENDER_SCREENSHOT_H

#include "download.hpp"

#include <vsg/core/Data.h>
#include <vsg/threading/OperationQueue.h>

namespace vsg
{
    class Window;
}
namespace Render
{
    struct Screenshot : public vsg::Operation
    {
        std::string writeFilename;
        vsg::ref_ptr<const vsg::Options> writeOptions;
        vsg::ref_ptr<vsg::Data> downloadData;

        vsg::ref_ptr<vsg::Device> device;
        vsg::ref_ptr<vsg::Image> srcImage;
        VkExtent2D srcExtent;
        VkFormat srcImageFormat;
        VkImageLayout srcImageLayout;

        Screenshot(vsg::ref_ptr<vsg::Device> in_device, vsg::ref_ptr<vsg::Image> in_srcImage, VkImageLayout in_srcImageLayout);
        Screenshot(vsg::ref_ptr<vsg::Window> window);

        void generateFilename(const std::string &path, const std::string &ext);
        void run() override;
    };
}

#endif
