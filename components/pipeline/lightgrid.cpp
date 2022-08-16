#include "lightgrid.hpp"

#include <components/vsgutil/share.hpp>
#include <components/vsgutil/readshader.hpp>

#include "layout.hpp"
#include "computebindings.hpp"

namespace Pipeline
{
    vsg::ref_ptr<vsg::BindComputePipeline> createUpdateLightGrid(vsg::ref_ptr<const vsg::Options> shaderOptions)
    {
        auto computeStage = VK_SHADER_STAGE_COMPUTE_BIT;
        vsg::PushConstantRanges pushConstantRanges{
            {computeStage, 0, getCompatiblePushConstantSize()}
        };

        vsg::DescriptorSetLayoutBindings bindings{
            {Descriptors::STORAGE_ARGS_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1, computeStage, nullptr},
        };
        vsg::DescriptorSetLayouts descriptorSetLayouts = getCompatibleDescriptorSetLayouts();
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(vsg::DescriptorSetLayoutBindings()));
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(bindings));

        auto layout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);
        auto shaderStage = vsgUtil::readShader("comp/light/assign.comp", shaderOptions);
        return vsg::BindComputePipeline::create(vsg::ComputePipeline::create(layout, shaderStage));
    }

    vsg::ref_ptr<vsg::BindComputePipeline> lightGrid(vsg::ref_ptr<const vsg::Options> shaderOptions)
    {
        return vsgUtil::share<vsg::BindComputePipeline>(createUpdateLightGrid, shaderOptions);
    }
}
