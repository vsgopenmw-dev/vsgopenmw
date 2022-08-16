#ifndef VSGOPENMW_VSGUTIL_NULLBIN_H
#define VSGOPENMW_VSGUTIL_NULLBIN_H

#include <vsg/nodes/Bin.h>

namespace vsgUtil
{
    /*
     * Removes traversal.
     */
    class NullBin : public vsg::Inherit<vsg::Bin, NullBin>
    {
    public:
        NullBin(uint32_t number) : Inherit(number, vsg::Bin::NO_SORT) {}
        void traverse(vsg::RecordTraversal &visitor) const override {}
    };
}

#endif
