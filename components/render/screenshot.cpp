#include "screenshot.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>

#include <vsg/app/Window.h>
#include <vsg/io/write.h>

namespace Render
{
    Screenshot::Screenshot(
        vsg::Device* in_device, vsg::ref_ptr<vsg::Image> in_srcImage, VkImageLayout in_srcImageLayout)
        : device(in_device)
        , srcImage(in_srcImage)
        , srcRect{ { 0, 0 }, { in_srcImage->extent.width, in_srcImage->extent.height } }
        , srcImageFormat(srcImage->format)
        , srcImageLayout(in_srcImageLayout)
    {
    }

    Screenshot::Screenshot(vsg::ref_ptr<vsg::Window> window)
        : device(window->getOrCreateDevice())
    {
        srcImageFormat = window->getOrCreateSwapchain()->getImageFormat();
        srcRect = { { 0, 0 }, window->extent2D() };
        // get the colour buffer image of the previous rendered frame as the current frame hasn't been rendered yet. The
        // 1 in window->imageIndex(1) means image from 1 frame ago.
        srcImage = window->imageView(window->imageIndex(1))->image;
        srcImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    void Screenshot::operate()
    {
        downloadData = download(device, srcImage, srcRect, srcImageFormat, srcImageLayout);
        if (!writeFilename.empty() && downloadData)
            vsg::write(downloadData, writeFilename.string(), writeOptions);
    }

    void Screenshot::generateFilename(const std::filesystem ::path& path, const std::string& ext)
    {
        int shotCount = 0;
        do
        {
            std::ostringstream stream;
            stream << std::setw(3) << std::setfill('0') << shotCount++ << "." << ext;
            writeFilename = path / ("screenshot" + stream.str());
        } while (std::filesystem::exists(writeFilename));
    }
}
