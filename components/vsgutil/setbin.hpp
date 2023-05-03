#ifndef VSGOPENMW_VSGUTIL_SETBIN_H
#define VSGOPENMW_VSGUTIL_SETBIN_H

#include <vsg/nodes/DepthSorted.h>

namespace vsgUtil
{
    /*
     * Doesn't cull.
     */
    const auto omniSphere = vsg::dsphere(vsg::dvec3(), std::numeric_limits<double>::infinity());
    class SetBin : public vsg::Inherit<vsg::DepthSorted, SetBin>
    {
    public:
        SetBin(int bin, vsg::ref_ptr<vsg::Node> child)
            : Inherit(bin, omniSphere, child)
        {
        }
    };
}

#endif
