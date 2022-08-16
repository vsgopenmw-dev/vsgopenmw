#include "worldoverlay.hpp"

#include <components/vsgutil/readshader.hpp>

#include "sets.hpp"

namespace Pipeline
{
    vsg::ref_ptr<vsg::BindComputePipeline> worldOverlay(vsg::ref_ptr<const vsg::Options> shaderOptions)
    {
        auto computeStage = VK_SHADER_STAGE_COMPUTE_BIT;
        vsg::DescriptorSetLayoutBindings textureBindings{
            {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, computeStage, nullptr},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, computeStage, nullptr},
        };
        vsg::DescriptorSetLayoutBindings computeBindings{
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, computeStage, nullptr},
        };
        vsg::DescriptorSetLayouts descriptorSetLayouts;
        for (int i=0; i<Pipeline::TEXTURE_SET; ++i)
            descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(vsg::DescriptorSetLayoutBindings()));
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(textureBindings));
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(computeBindings));

        vsg::PushConstantRanges pushConstantRanges{
            {computeStage, 0, sizeof(vsg::vec4)}
        };
        auto layout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

        auto shaderStage = vsgUtil::readShader("comp/worldoverlay/update.comp", shaderOptions);
        return vsg::BindComputePipeline::create(vsg::ComputePipeline::create(layout, shaderStage));
    }
}
