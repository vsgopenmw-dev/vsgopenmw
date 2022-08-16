#ifndef VSGOPENMW_MWRENDER_IMAGEIO_H
#define VSGOPENMW_MWRENDER_IMAGEIO_H

#include <vsg/core/Data.h>

namespace MWRender
{
    /*nothrow*/ vsg::ref_ptr<vsg::Data> readImageFromMemory(const std::vector<char> &encoded);
    /*nothrow*/ std::vector<char> writeImageToMemory(vsg::ref_ptr<vsg::Data> data);
}

#endif
