#ifndef VSGOPENMW_VSGUTIL_EXTERNALBIN_H
#define VSGOPENMW_VSGUTIL_EXTERNALBIN_H

#include <vsg/nodes/Bin.h>
#include <vsg/threading/Latch.h>

namespace vsgUtil
{
    /*
     * Moves traversal.
     */
    class ExternalBin : public vsg::Bin
    {
    public:
        void traverse(vsg::RecordTraversal &visitor) const override;
        void execute(vsg::RecordTraversal &visitor) const;
        vsg::ref_ptr<vsg::Latch> latch = vsg::Latch::create(1);
    };

    class ExecuteBins : public vsg::Node
    {
    public:
        std::vector<vsg::ref_ptr<ExternalBin>> bins;
        void accept(vsg::RecordTraversal &) const override;
        vsg::Mask mask = vsg::MASK_ALL;
    };
}

#endif
