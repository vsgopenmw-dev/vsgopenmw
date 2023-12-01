#ifndef VSGOPENMW_VSGUTIL_NULLBIN_H
#define VSGOPENMW_VSGUTIL_NULLBIN_H

#include <vsg/nodes/Bin.h>

namespace vsgUtil
{
    /*
     * NullBin is a special bin that is not traversed so that all contents placed into the bin are discarded.
     * Useful for turning off certain effects/nodes in specific views.
     */
    class NullBin : public vsg::Inherit<vsg::Bin, NullBin>
    {
    public:
        NullBin(uint32_t number)
            : Inherit(number, vsg::Bin::NO_SORT)
        {
        }
        void traverse(vsg::RecordTraversal& visitor) const override {}
    };
}

#endif
