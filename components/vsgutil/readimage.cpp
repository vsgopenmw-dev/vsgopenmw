#include "readimage.hpp"

#include <vsg/io/read.h>
#include <vsg/state/DescriptorImage.h>

#include <iostream>

namespace vsgUtil
{
    vsg::ref_ptr<vsg::Data> readImage(const std::string &path, vsg::ref_ptr<const vsg::Options> options)
    {
        auto data = vsg::read_cast<vsg::Data>(path, options);
        if (!data)
        {
            std::cerr << "!vsg::read_cast<vsg::Data>(\"" + path + "\")" << std::endl;
            static const auto image = vsg::ubvec4Array2D::create(1,1, vsg::ubvec4(0xff,0x00,0xff,0xff), vsg::Data::Layout{.format=VK_FORMAT_R8G8B8A8_UNORM});
            return image;
        }
        return data;
    }

    vsg::ref_ptr<vsg::Data> readOptionalImage(const std::string &path, vsg::ref_ptr<const vsg::Options> options)
    {
        return vsg::read_cast<vsg::Data>(path, options);
    }

    vsg::ref_ptr<vsg::DescriptorImage> createDescriptorImage(vsg::ref_ptr<vsg::Data> image, VkSamplerAddressMode addressMode, uint32_t binding)
    {
        auto sampler = vsg::Sampler::create();
        sampler->addressModeU = sampler->addressModeV = addressMode;
        return vsg::DescriptorImage::create(sampler, image, binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    vsg::ref_ptr<vsg::DescriptorImage> readDescriptorImage(const std::string &path, vsg::ref_ptr<const vsg::Options> options, VkSamplerAddressMode addressMode, uint32_t binding)
    {
        return createDescriptorImage(readImage(path, options), addressMode, binding);
    }
}
