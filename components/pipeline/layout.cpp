#include "layout.hpp"

#include "scenedata.hpp"
#include "lightstages.hpp"

namespace Pipeline
{
   vsg::DescriptorSetLayouts getCompatibleDescriptorSetLayouts()
    {
       static const auto objectLayout = vsg::DescriptorSetLayout::create(vsg::DescriptorSetLayoutBindings{
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}});
        return {getViewDescriptorSetLayout(), objectLayout};
    }

    vsg::ref_ptr<vsg::DescriptorSetLayout> getViewDescriptorSetLayout()
    {
        static const auto sceneLayout = vsg::DescriptorSetLayout::create(vsg::DescriptorSetLayoutBindings{
            {Descriptors::VIEW_SCENE_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, lightStages|VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
            {Descriptors::VIEW_ENV_BINDING, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {Descriptors::VIEW_LIGHTS_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1, lightStages|VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
            {Descriptors::VIEW_LIGHT_GRID_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, lightStages|VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
            {Descriptors::VIEW_LIGHT_INDICES_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, lightStages|VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
            {Descriptors::VIEW_LIGHT_GRID_SIZE_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, lightStages|VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
            {Descriptors::VIEW_SHADOW_MAP_BINDING, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
        });
        return sceneLayout;
    }

    vsg::ref_ptr<vsg::PipelineLayout> getCompatiblePipelineLayout()
    {
        static const auto layout = vsg::PipelineLayout::create(
            getCompatibleDescriptorSetLayouts(),
            vsg::PushConstantRanges {{VK_SHADER_STAGE_VERTEX_BIT, 0, getCompatiblePushConstantSize()}}
        );
        return layout;
    }
}
