#ifndef VSGOPENMW_VSGUTIL_SHAREIMAGE_H
#define VSGOPENMW_VSGUTIL_SHAREIMAGE_H

#include <vsg/state/DescriptorImage.h>
#include <vsg/utils/SharedObjects.h>

#include "share.hpp"

namespace vsgUtil
{
    /*
     * Adapts convenience constructors to SharedObjects.
     */

    vsg::ref_ptr<vsg::ImageInfo> sharedImageInfo(vsg::ref_ptr<vsg::SharedObjects> sharedObjects, vsg::ref_ptr<vsg::Sampler>& sampler, vsg::ref_ptr<vsg::Data> data);

    vsg::ref_ptr<vsg::DescriptorImage> sharedDescriptorImage(vsg::ref_ptr<vsg::SharedObjects> sharedObjects, vsg::ref_ptr<vsg::Sampler>& sampler, vsg::ref_ptr<vsg::Data> data, uint32_t dstBinding=0);
}

#endif
