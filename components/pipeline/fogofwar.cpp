#include "fogofwar.hpp"

#include <components/vsgutil/readshader.hpp>

#include "sets.hpp"

namespace
{
#include <files/shaders/comp/fogofwar/constants.glsl>
}
namespace Pipeline
{
    vsg::ref_ptr<vsg::BindComputePipeline> fogOfWar(vsg::ref_ptr<const vsg::Options> shaderOptions)
    {
        auto computeStage = VK_SHADER_STAGE_COMPUTE_BIT;
        vsg::DescriptorSetLayoutBindings bindings{
            {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, computeStage, nullptr},
        };
        vsg::DescriptorSetLayouts descriptorSetLayouts;
        for (int i=0; i<Pipeline::COMPUTE_SET; ++i)
            descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(vsg::DescriptorSetLayoutBindings()));
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(bindings));

        vsg::PushConstantRanges pushConstantRanges{
            {computeStage, 0, 2*sizeof(vsg::vec4)}
        };
        auto layout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

        auto shaderStage = vsgUtil::readShader("comp/fogofwar/update.comp", shaderOptions);
        return vsg::BindComputePipeline::create(vsg::ComputePipeline::create(layout, shaderStage));
    }
}
