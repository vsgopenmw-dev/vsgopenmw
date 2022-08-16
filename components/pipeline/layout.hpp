#ifndef VSGOPENMW_PIPELINE_LAYOUT_H
#define VSGOPENMW_PIPELINE_LAYOUT_H

#include <vsg/state/DescriptorSetLayout.h>
#include <vsg/state/PipelineLayout.h>

namespace Pipeline
{
    /*
     * Allows descriptor sets to remain bound across pipelines.
     */
    vsg::DescriptorSetLayouts getCompatibleDescriptorSetLayouts();
    vsg::ref_ptr<vsg::DescriptorSetLayout> getViewDescriptorSetLayout();
    vsg::ref_ptr<vsg::PipelineLayout> getCompatiblePipelineLayout();
    inline uint32_t getCompatiblePushConstantSize() { return 128; }
}

#endif
