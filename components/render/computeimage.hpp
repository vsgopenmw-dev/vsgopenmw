#ifndef VSGOPENMW_RENDER_COMPUTEIMAGE_H
#define VSGOPENMW_RENDER_COMPUTEIMAGE_H

#include <vsg/nodes/Group.h>
#include <vsg/commands/Dispatch.h>
#include <vsg/state/Image.h>

namespace vsg
{
    class PipelineBarrier;
}
namespace Render
{
    /*
     * Provides base class for computing images on the GPU.
     */
    class ComputeImage : public vsg::Group
    {
    protected:
        vsg::ref_ptr<vsg::PipelineBarrier> mPreComputeBarrier;
        vsg::ref_ptr<vsg::PipelineBarrier> mPostComputeBarrier;
        vsg::ref_ptr<vsg::PipelineBarrier> mPreClearBarrier;
        vsg::ref_ptr<vsg::PipelineBarrier> mPostClearBarrier;
        vsg::ref_ptr<vsg::Dispatch> mDispatch;

        void clear(vsg::ref_ptr<vsg::Image> image, vsg::RecordTraversal &visitor) const;
        void handleClearRequests(vsg::RecordTraversal &visitor) const;
        void disableDefaultPushConstants(vsg::RecordTraversal &visitor) const;
        void compute(vsg::RecordTraversal &visitor, vsg::ref_ptr<vsg::Image> image, uint32_t groupCountX, uint32_t groupCountY) const;
        VkClearColorValue mClearColor;
    public:
        ComputeImage();
        ~ComputeImage();

        struct ClearRequest
        {
            vsg::ref_ptr<vsg::Image> image;
        };
        mutable std::vector<ClearRequest> clearRequests;
    };
}

#endif
