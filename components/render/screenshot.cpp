#include "screenshot.hpp"

#include <iostream>
#include <iomanip>
#include <filesystem>

#include <vsg/io/write.h>
#include <vsg/core/Exception.h>
#include <vsg/viewer/Window.h>

namespace Render
{
    Screenshot::Screenshot(vsg::ref_ptr<vsg::Device> in_device, vsg::ref_ptr<vsg::Image> in_srcImage, VkImageLayout in_srcImageLayout)
        : device(in_device), srcImage(in_srcImage), srcExtent{in_srcImage->extent.width, in_srcImage->extent.height}, srcImageFormat(srcImage->format), srcImageLayout(in_srcImageLayout)
    {
    }

    Screenshot::Screenshot(vsg::ref_ptr<vsg::Window> window)
        : device(window->getOrCreateDevice())
    {
        srcImageFormat = window->getOrCreateSwapchain()->getImageFormat();
        srcExtent = window->extent2D();
        // get the colour buffer image of the previous rendered frame as the current frame hasn't been rendered yet.  The 1 in window->imageIndex(1) means image from 1 frame ago.
        srcImage = window->imageView(window->imageIndex(1))->image;
        srcImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    void Screenshot::run()
    {
        try
        {
            downloadData = download(device, srcImage, srcExtent, srcImageFormat, srcImageLayout);
            if (!writeFilename.empty() && downloadData)
                vsg::write(downloadData, writeFilename, writeOptions);
        }
        catch(std::exception &e)
        {
            std::cerr << "!Screenshot(" << e.what() << ")" <<std::endl;
        }
        catch(vsg::Exception &e)
        {
            std::cerr << "!Screenshot(vsg::Exception(" << e.message << "))" <<std::endl;
        }
    }

    void Screenshot::generateFilename(const std::string &path, const std::string &ext)
    {
        int shotCount = 0;
        do
        {
            std::ostringstream stream;
            stream << path << "/screenshot" << std::setw(3) << std::setfill('0') << shotCount++ << "." << ext;
            writeFilename = stream.str();
        } while (std::filesystem::exists(writeFilename));
    }
}
