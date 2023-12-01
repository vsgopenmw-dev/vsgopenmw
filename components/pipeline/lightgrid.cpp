#include "lightgrid.hpp"

#include <components/vsgutil/readshader.hpp>
#include <components/vsgutil/share.hpp>

#include "computebindings.hpp"
#include "layout.hpp"

namespace Pipeline
{
    vsg::ref_ptr<vsg::BindComputePipeline> createUpdateLightGrid(vsg::ref_ptr<const vsg::Options> shaderOptions)
    {
        vsg::DescriptorSetLayoutBindings bindings{
            { Descriptors::STORAGE_ARGS_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
                nullptr },
        };
        vsg::DescriptorSetLayouts descriptorSetLayouts = getCompatibleDescriptorSetLayouts();
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(vsg::DescriptorSetLayoutBindings()));
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(bindings));

        auto layout = vsg::PipelineLayout::create(descriptorSetLayouts, vsg::PushConstantRanges());
        auto shaderStage = vsgUtil::readShader("comp/light/assign.comp", shaderOptions);
        return vsg::BindComputePipeline::create(vsg::ComputePipeline::create(layout, shaderStage));
    }

    vsg::ref_ptr<vsg::BindComputePipeline> lightGrid(vsg::ref_ptr<const vsg::Options> shaderOptions)
    {
        return vsgUtil::share<vsg::BindComputePipeline>(createUpdateLightGrid, shaderOptions);
    }
}
