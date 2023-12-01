#ifndef VSGOPENMW_VSGUTIL_IMAGE_H
#define VSGOPENMW_VSGUTIL_IMAGE_H

#include <vsg/state/ImageView.h>

namespace vsgUtil
{
    /*
     * Adds a convenience constructor for vsg::Image.
     */
    inline vsg::ref_ptr<vsg::Image> createImage(VkFormat format, const VkExtent2D& extent, VkImageUsageFlags usage, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
    {
        auto image = vsg::Image::create();
        image->imageType = VK_IMAGE_TYPE_2D;
        image->format = format;
        image->extent = { extent.width, extent.height, 1 };
        image->usage = usage;
        image->samples = samples;
        image->mipLevels = 1;
        image->arrayLayers = 1;
        return image;
    }

    template <typename... Args>
    vsg::ref_ptr<vsg::ImageView> createImageAndView(Args&&... args)
    {
        return vsg::ImageView::create(createImage(args...));
    }
}

#endif
