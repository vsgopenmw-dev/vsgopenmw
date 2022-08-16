#include "externalbin.hpp"

namespace vsgUtil
{
    void ExternalBin::traverse(vsg::RecordTraversal &visitor) const
    {
        latch->count_down();
    }

    void ExternalBin::execute(vsg::RecordTraversal &visitor) const
    {
        latch->wait();
        Bin::traverse(visitor);
        latch->count_up();
    }

    void ExecuteBins::accept(vsg::RecordTraversal &visitor) const
    {
        if (mask & visitor.traversalMask)
            for (auto &bin : bins)
                bin->execute(visitor);
    }
}
