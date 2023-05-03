#ifndef VSGOPENMW_VSGUTIL_READIMAGE_H
#define VSGOPENMW_VSGUTIL_READIMAGE_H

#include <vsg/io/Options.h>

namespace vsgUtil
{
    /*
     * Reads an image or the default image on failure.
     */
    vsg::ref_ptr<vsg::Data> readImage(const std::string& path, vsg::ref_ptr<const vsg::Options> options);

    vsg::ref_ptr<vsg::Data> readOptionalImage(const std::string& path, vsg::ref_ptr<const vsg::Options> options);
}

#endif
