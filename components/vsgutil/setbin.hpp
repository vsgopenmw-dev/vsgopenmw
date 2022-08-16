#ifndef VSGOPENMW_VSGUTIL_SETBIN_H
#define VSGOPENMW_VSGUTIL_SETBIN_H

#include <vsg/nodes/DepthSorted.h>

namespace vsgUtil
{
    /*
     * Doesn't cull.
     */
    class SetBin : public vsg::Inherit<vsg::DepthSorted, SetBin>
    {
    public:
        SetBin(int bin, vsg::ref_ptr<vsg::Node> child)
            : Inherit(bin, vsg::dsphere(vsg::vec3(),99999999), child) {}
    };
}

#endif
