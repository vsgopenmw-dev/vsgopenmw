#ifndef VSGOPENMW_VSGUTIL_DELETIONQUEUE_H
#define VSGOPENMW_VSGUTIL_DELETIONQUEUE_H

#include <vsg/core/Object.h>

namespace vsgUtil
{
    /*
     * Prevents Vulkan objects from being deleted while still in use by a queued frame.
     */
    class DeletionQueue
    {
        using Queue = std::vector<vsg::ref_ptr<vsg::Object>>;
        std::vector<Queue> _queues;
    public:
        DeletionQueue(size_t numFrames)
        {
            _queues.resize(numFrames);
        }
        void add(vsg::ref_ptr<vsg::Object> object)
        {
            _queues[0].push_back(object);
        }
        void advanceFrame()
        {
            _queues.back().clear();
            for (size_t i = _queues.size()-1; i > 0; --i)
                _queues[i].swap(_queues[i-1]);
        }
    };
}

#endif
