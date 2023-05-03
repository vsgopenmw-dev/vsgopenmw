#include "shareimage.hpp"

#include <vsg/io/Options.h>

namespace vsgUtil
{
    vsg::ref_ptr<vsg::ImageInfo> sharedImageInfo(vsg::ref_ptr<vsg::SharedObjects> sharedObjects, vsg::ref_ptr<vsg::Sampler>& sampler, vsg::ref_ptr<vsg::Data> data)
    {
        share_if(sharedObjects, sampler);

        auto image = vsg::Image::create(data);
        image->usage |= (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        share_if(sharedObjects, image);

        auto imageView = vsg::ImageView::create(image);
        share_if(sharedObjects, imageView);

        auto imageInfo = vsg::ImageInfo::create(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        share_if(sharedObjects, imageInfo);
        return imageInfo;
    }

    vsg::ref_ptr<vsg::DescriptorImage> sharedDescriptorImage(vsg::ref_ptr<vsg::SharedObjects> sharedObjects, vsg::ref_ptr<vsg::Sampler>& sampler, vsg::ref_ptr<vsg::Data> data, uint32_t dstBinding)
    {
        auto imageInfo = sharedImageInfo(sharedObjects, sampler, data);
        auto descriptor = vsg::DescriptorImage::create(imageInfo, dstBinding);
        share_if(sharedObjects, descriptor);
        return descriptor;
    }
}
