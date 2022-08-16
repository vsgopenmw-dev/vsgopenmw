#include "terrain.hpp"

#include <vsg/state/VertexInputState.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/state/MultisampleState.h>
#include <vsg/state/DepthStencilState.h>

#include <components/vsgutil/readshader.hpp>

#include "layout.hpp"

namespace Pipeline
{
    namespace Attributes
    {
        #include <files/shaders/terrain/attributes.glsl>
    }

    vsg::ref_ptr<vsg::BindGraphicsPipeline> Terrain::create(const TerrainKey &key) const
    {
        vsg::ShaderStages shaders{
            vsgUtil::readShader("terrain/main.vert", mShaderOptions),
            vsgUtil::readShader("terrain/main.frag", mShaderOptions)};

        shaders[1]->specializationConstants = vsg::ShaderStage::SpecializationConstants{{0, vsg::intValue::create(key.layerCount)}};

        vsg::DescriptorSetLayoutBindings descriptorBindings {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, key.layerCount, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, key.layerCount, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};

        vsg::DescriptorSetLayouts descriptorSetLayouts = getCompatibleDescriptorSetLayouts();
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(descriptorBindings));

        vsg::PushConstantRanges pushConstantRanges{
            {VK_SHADER_STAGE_VERTEX_BIT, 0, getCompatiblePushConstantSize()}
        };

        uint32_t vertexBindingIndex = 0;
        vsg::VertexInputState::Bindings vertexBindingsDescriptions;
        vsg::VertexInputState::Attributes vertexAttributeDescriptions;
        {
            vertexBindingsDescriptions.push_back({vertexBindingIndex, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributeDescriptions.push_back({Attributes::VERTEX_LOCATION, vertexBindingIndex++, VK_FORMAT_R32G32B32_SFLOAT, 0});
        }
        {
            vertexBindingsDescriptions.push_back({vertexBindingIndex, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributeDescriptions.push_back({Attributes::NORMAL_LOCATION, vertexBindingIndex++, VK_FORMAT_R32G32B32_SFLOAT, 0});
        }
        {
            vertexBindingsDescriptions.push_back({vertexBindingIndex, sizeof(vsg::ubvec4), VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributeDescriptions.push_back({Attributes::COLOR_LOCATION, vertexBindingIndex++, VK_FORMAT_R8G8B8A8_UNORM, 0});
        }
        {
            vertexBindingsDescriptions.push_back({vertexBindingIndex, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributeDescriptions.push_back({Attributes::TEXCOORD_LOCATION, vertexBindingIndex++, VK_FORMAT_R32G32_SFLOAT, 0});
        }

        auto pipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

        vsg::ColorBlendState::ColorBlendAttachments colorBlendAttachments;
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.blendEnable = false;
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                              VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT |
                                              VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachments.push_back(colorBlendAttachment);

        vsg::GraphicsPipelineStates pipelineStates{
            vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
            vsg::InputAssemblyState::create(),
            vsg::RasterizationState::create(),
            vsg::MultisampleState::create(),
            vsg::ColorBlendState::create(colorBlendAttachments),
            vsg::DepthStencilState::create()};

        auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(vsg::GraphicsPipeline::create(pipelineLayout, shaders, pipelineStates));
        return bindGraphicsPipeline;
    }
};
