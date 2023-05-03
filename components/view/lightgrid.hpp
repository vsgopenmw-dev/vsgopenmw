#ifndef VSGOPENMW_VIEW_LIGHTGRID_H
#define VSGOPENMW_VIEW_LIGHTGRID_H

#include <components/pipeline/descriptorvalue.hpp>
#include <components/vsgutil/composite.hpp>

namespace View
{
    /*
     * Narrows light selection.
     */
    class LightGrid : public vsgUtil::Composite<vsg::Node>
    {
        Pipeline::DynamicDescriptorValue<unsigned int> mIndexCount;

    public:
        LightGrid(const vsg::Options& shaderOptions, vsg::ref_ptr<vsg::DescriptorSet> viewSet);
        void update() { mIndexCount.dirty(); }
    };
}

#endif
