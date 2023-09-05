#ifndef VSGOPENMW_VSGUTIL_IMAGEIO_H
#define VSGOPENMW_VSGUTIL_IMAGEIO_H

#include <vsg/core/Data.h>

namespace vsgUtil
{
    /*nothrow*/ vsg::ref_ptr<vsg::Data> readImageFromMemory(const std::vector<char>& encoded);
    /*nothrow*/ std::vector<char> writeImageToMemory(vsg::ref_ptr<vsg::Data> data, const char* format = ".png");
}

#endif
