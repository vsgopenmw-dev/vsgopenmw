#include "lightgrid.hpp"

#include <vsg/commands/Dispatch.h>
#include <vsg/io/Options.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/state/BindDescriptorSet.h>

#include <components/pipeline/computebindings.hpp>
#include <components/pipeline/layout.hpp>
#include <components/pipeline/lightgrid.hpp>
#include <components/pipeline/sets.hpp>

#include "gridsize.hpp"

namespace View
{
    LightGrid::LightGrid(const vsg::Options& shaderOptions, vsg::ref_ptr<vsg::DescriptorSet> viewSet)
        : mIndexCount(Pipeline::Descriptors::STORAGE_ARGS_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
    {
        auto bindComputePipeline = Pipeline::lightGrid(vsg::ref_ptr{ &shaderOptions });

        auto layout = bindComputePipeline->pipeline->layout;
        auto descriptorSet = vsg::DescriptorSet::create(
            layout->setLayouts[Pipeline::COMPUTE_SET], vsg::Descriptors{ mIndexCount.descriptor() });
        auto bindDescriptorSet = vsg::BindDescriptorSet::create(
            VK_PIPELINE_BIND_POINT_COMPUTE, layout, Pipeline::COMPUTE_SET, descriptorSet);

        auto bindViewSet
            = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_COMPUTE, layout, Pipeline::VIEW_SET, viewSet);
        auto sg = vsg::StateGroup::create();
        sg->stateCommands = { bindComputePipeline, bindDescriptorSet };
        sg->children = { bindViewSet, vsg::Dispatch::create(gridSizeX, gridSizeY, gridSizeZ) };
        mNode = sg;
    }
}
