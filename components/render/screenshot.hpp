#ifndef VSGOPENMW_RENDER_SCREENSHOT_H
#define VSGOPENMW_RENDER_SCREENSHOT_H

#include <filesystem>

#include "download.hpp"

#include <vsg/core/Data.h>

#include <components/vsgutil/operation.hpp>

namespace vsg
{
    class Window;
}
namespace Render
{
    struct Screenshot : public vsgUtil::Operation
    {
        std::filesystem::path writeFilename;
        vsg::ref_ptr<const vsg::Options> writeOptions;
        vsg::ref_ptr<vsg::Data> downloadData;

        vsg::Device* device;
        vsg::ref_ptr<vsg::Image> srcImage;
        VkRect2D srcRect;
        VkFormat srcImageFormat;
        VkImageLayout srcImageLayout;

        Screenshot(
            vsg::Device* in_device, vsg::ref_ptr<vsg::Image> in_srcImage, VkImageLayout in_srcImageLayout);
        Screenshot(vsg::ref_ptr<vsg::Window> window);

        void generateFilename(const std::filesystem::path& path, const std::string& ext);
        void operate() override;
    };
}

#endif
