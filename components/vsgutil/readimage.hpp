#ifndef VSGOPENMW_VSGUTIL_READIMAGE_H
#define VSGOPENMW_VSGUTIL_READIMAGE_H

#include <vsg/io/Options.h>

namespace vsgUtil
{
    /*
     * Reads an image or the default image on failure.
     */
    vsg::ref_ptr<vsg::Data> readImage(const std::string &path, vsg::ref_ptr<const vsg::Options> options);

    vsg::ref_ptr<vsg::Data> readOptionalImage(const std::string &path, vsg::ref_ptr<const vsg::Options> options);

    /*
     * Conveniently creates a combined image sampler.
     */
    vsg::ref_ptr<vsg::DescriptorImage> createDescriptorImage(vsg::ref_ptr<vsg::Data> image, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, uint32_t binding=0);

    /*
     * Conveniently reads image into a combined image sampler.
     */
    vsg::ref_ptr<vsg::DescriptorImage> readDescriptorImage(const std::string &path, vsg::ref_ptr<const vsg::Options> options, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, uint32_t binding=0);
}

#endif
