#include "defaultstate.hpp"

#include <vsg/state/BindDescriptorSet.h>

#include <components/pipeline/layout.hpp>
#include <components/pipeline/object.hpp>
#include <components/pipeline/sets.hpp>

namespace View
{
    vsg::ref_ptr<vsg::StateGroup> createDefaultState(vsg::ref_ptr<vsg::DescriptorSet> viewDescriptorSet)
    {
        auto sg = vsg::StateGroup::create();
        auto layout = Pipeline::getCompatiblePipelineLayout();
        auto objectSet = vsg::DescriptorSet::create(
            layout->setLayouts[Pipeline::OBJECT_SET], vsg::Descriptors{ Pipeline::Object(0).descriptor() });
        sg->stateCommands = { vsg::BindDescriptorSet::create(
                                  VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::VIEW_SET, viewDescriptorSet),
            vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::OBJECT_SET, objectSet) };
        return sg;
    }
}
