#ifndef VSGOPENMW_VSGUTIL_SETBIN_H
#define VSGOPENMW_VSGUTIL_SETBIN_H

#include <vsg/nodes/DepthSorted.h>

namespace vsgUtil
{
    /*
     * Doesn't cull.
     */
    const auto omniSphere = vsg::dsphere(vsg::dvec3(), std::numeric_limits<double>::infinity());

    inline vsg::ref_ptr<vsg::DepthSorted> createBinSetter(int bin, vsg::ref_ptr<vsg::Node> child)
    {
        return vsg::DepthSorted::create(bin, omniSphere, child);
    }
}

#endif
