#include "lightgrid.hpp"

#include <vsg/commands/Dispatch.h>
#include <vsg/commands/Commands.h>
#include <vsg/io/Options.h>
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
        auto bindComputeSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_COMPUTE, layout, Pipeline::COMPUTE_SET, descriptorSet);

        auto bindViewSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_COMPUTE, layout, Pipeline::VIEW_SET, viewSet);

        #include <files/shaders/comp/light/workgroupsize.glsl>
        // ensure there are no remainders to avoid the need for range checking the workGroupID in the compute shader
        static_assert(gridSizeX % workGroupSizeX == 0);
        static_assert(gridSizeY % workGroupSizeY == 0);
        static_assert(gridSizeZ % workGroupSizeZ == 0);

        // Vulkan guarantees a minimum of 128 maxComputeWorkGroupInvocations.
        static_assert(workGroupSizeX * workGroupSizeY * workGroupSizeZ <= 128);

        auto dispatch = vsg::Dispatch::create(gridSizeX/workGroupSizeX, gridSizeY/workGroupSizeY, gridSizeZ/workGroupSizeZ);

        auto commands = vsg::Commands::create(); // vsg::StateGroup::create();
        commands->children = { bindComputePipeline, bindComputeSet, bindViewSet, dispatch };
        mNode = commands;
    }
}
