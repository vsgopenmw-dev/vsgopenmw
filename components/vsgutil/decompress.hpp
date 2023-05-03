#ifndef VSGOPENMW_VSGUTIL_DECOMPRESS_H
#define VSGOPENMW_VSGUTIL_DECOMPRESS_H

#include <vsg/core/Data.h>

namespace vsgUtil
{
    /*
     * Unpacks image if in compressed format.
     */
    vsg::ref_ptr<vsg::Data> decompressImage(const vsg::Data& in_data);
    bool isCompressed(const vsg::Data& in_data);
}

#endif
