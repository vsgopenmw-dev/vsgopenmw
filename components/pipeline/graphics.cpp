#include "graphics.hpp"

#include <vsg/state/VertexInputState.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/state/MultisampleState.h>
#include <vsg/state/DepthStencilState.h>

#include <components/vsgutil/readshader.hpp>

#include "bindings.hpp"
#include "layout.hpp"

namespace Pipeline
{
    std::string getDefine(Mode mode)
    {
        switch (mode)
        {
            case Mode::MATERIAL: return "MATERIAL";
            case Mode::TEXMAT: return "TEXMAT";
            case Mode::DIFFUSE_MAP: return "DIFFUSE_MAP";
            case Mode::DARK_MAP: return "DARK_MAP";
            case Mode::DETAIL_MAP: return "DETAIL_MAP";
            case Mode::DECAL_MAP: return "DECAL_MAP";
            case Mode::GLOW_MAP: return "GLOW_MAP";
            case Mode::BUMP_MAP: return "BUMP_MAP";
            case Mode::ENV_MAP: return "ENV_MAP";
            case Mode::BILLBOARD: return "BILLBOARD";
            case Mode::PARTICLE: return "PARTICLE";
            case Mode::MORPH: return "MORPH";
        }
    }
    std::string getDefine(GeometryMode mode)
    {
        switch (mode)
        {
            case GeometryMode::VERTEX: return "VERTEX";
            case GeometryMode::NORMAL: return "NORMAL";
            case GeometryMode::COLOR: return "COLOR";
            case GeometryMode::TEXCOORD: return "TEXCOORD";
            case GeometryMode::SKIN: return "SKIN";
        }
    }

    vsg::ref_ptr<vsg::BindGraphicsPipeline> Graphics::create(const Options &key) const
    {
        auto numUvSets = key.numUvSets;
        auto compileSettings = vsg::ShaderCompileSettings::create();

        for (size_t i=0; i<key.modes.size(); ++i)
        {
            if (key.modes[i])
                compileSettings->defines.emplace_back(getDefine(static_cast<Mode>(i)));
        }
        for (size_t i=0; i<key.geometryModes.size(); ++i)
        {
            if (key.geometryModes[i])
                compileSettings->defines.emplace_back(getDefine(static_cast<GeometryMode>(i)));
        }
        auto shader = key.shader;
        if (shader.empty())
            shader = "default/";
        vsg::ShaderStages shaders{
            vsgUtil::readShader(shader + "main.vert", mShaderOptions),
            vsgUtil::readShader(shader + "main.frag", mShaderOptions)};
        //if (mShaderOptions.objectCache) shaderStage = ShaderStage::create(*origShaderStage);

        shaders[0]->module->hints = shaders[1]->module->hints = compileSettings;

        auto specConstants = vsg::ShaderStage::SpecializationConstants{{0, vsg::intValue::create(numUvSets)}};

        auto vertexSpecConstants = specConstants;
        for (auto [binding, uvSet] : key.nonStandardUvSets)
            if (binding == Descriptors::TEXMAT_BINDING)
                vertexSpecConstants.emplace(Descriptors::START_UVSET_CONSTANTS+binding, vsg::intValue::create(uvSet));

        shaders[0]->specializationConstants = vertexSpecConstants;

        auto fragmentSpecConstants = specConstants;
        fragmentSpecConstants.emplace(1, vsg::intValue::create(key.alphaTestMode));
        fragmentSpecConstants.emplace(2, vsg::intValue::create(key.colorMode));
        fragmentSpecConstants.emplace(3, vsg::boolValue::create(key.specular));
        for (auto [binding, uvSet] : key.nonStandardUvSets)
            if (binding != Descriptors::TEXMAT_BINDING && binding != Descriptors::ENV_UNIT)
                fragmentSpecConstants.emplace(Descriptors::START_UVSET_CONSTANTS+binding, vsg::intValue::create(uvSet));
        shaders[1]->specializationConstants = fragmentSpecConstants;

        vsg::DescriptorSetLayoutBindings descriptorBindings;
        if (key.hasMode(Mode::MATERIAL))
            descriptorBindings.push_back({Descriptors::MATERIAL_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        if (key.hasMode(Mode::PARTICLE))
            descriptorBindings.push_back({Descriptors::PARTICLE_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr});
        if (key.hasMode(Mode::MORPH))
        {
            descriptorBindings.push_back({Descriptors::MORPH_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr});
            descriptorBindings.push_back({Descriptors::MORPH_WEIGHTS_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr});
        }
        if (key.hasMode(GeometryMode::SKIN))
            descriptorBindings.push_back({Descriptors::BONE_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr});
        if (key.hasMode(Mode::DIFFUSE_MAP))
            descriptorBindings.push_back({Descriptors::DIFFUSE_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        if (key.hasMode(Mode::DARK_MAP))
            descriptorBindings.push_back({Descriptors::DARK_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        if (key.hasMode(Mode::DETAIL_MAP))
            descriptorBindings.push_back({Descriptors::DETAIL_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        if (key.hasMode(Mode::DECAL_MAP))
            descriptorBindings.push_back({Descriptors::DECAL_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        if (key.hasMode(Mode::GLOW_MAP))
            descriptorBindings.push_back({Descriptors::GLOW_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        if (key.hasMode(Mode::BUMP_MAP))
            descriptorBindings.push_back({Descriptors::BUMP_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        if (key.hasMode(Mode::ENV_MAP))
            descriptorBindings.push_back({Descriptors::ENV_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        if (key.hasMode(Mode::TEXMAT))
            descriptorBindings.push_back({Descriptors::TEXMAT_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr});

        vsg::DescriptorSetLayouts descriptorSetLayouts = getCompatibleDescriptorSetLayouts();
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(descriptorBindings));

        vsg::PushConstantRanges pushConstantRanges{
            {VK_SHADER_STAGE_VERTEX_BIT, 0, getCompatiblePushConstantSize()}
        };

        uint32_t vertexBindingIndex = 0;
        vsg::VertexInputState::Bindings vertexBindingsDescriptions;
        vsg::VertexInputState::Attributes vertexAttributeDescriptions;
        static_assert(static_cast<int>(GeometryMode::VERTEX) == 0);
        if (key.hasMode(GeometryMode::VERTEX))
        {
            vertexBindingsDescriptions.push_back({vertexBindingIndex, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributeDescriptions.push_back({vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32_SFLOAT, 0});
        }
        static_assert(static_cast<int>(GeometryMode::NORMAL) == 1);
        if (key.hasMode(GeometryMode::NORMAL))
        {
            vertexBindingsDescriptions.push_back({vertexBindingIndex, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributeDescriptions.push_back({vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32_SFLOAT, 0});
        }
        static_assert(static_cast<int>(GeometryMode::COLOR) == 2);
        if (key.hasMode(GeometryMode::COLOR))
        {
            vertexBindingsDescriptions.push_back({vertexBindingIndex, sizeof(vsg::vec4), VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributeDescriptions.push_back({vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32A32_SFLOAT, 0});
        }
        static_assert(static_cast<int>(GeometryMode::SKIN) == 3);
        if (key.hasMode(GeometryMode::SKIN))
        {
            vertexBindingsDescriptions.push_back({vertexBindingIndex, sizeof(vsg::vec4), VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributeDescriptions.push_back({vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32A32_SFLOAT, 0});

            vertexBindingsDescriptions.push_back({vertexBindingIndex, sizeof(vsg::vec4), VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributeDescriptions.push_back({vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32A32_SFLOAT, 0});
        }
        static_assert(static_cast<int>(GeometryMode::TEXCOORD) == 4);
        if (key.hasMode(GeometryMode::TEXCOORD))
        {
            for (uint32_t i=0; i<numUvSets; ++i)
            {
                vertexBindingsDescriptions.push_back({vertexBindingIndex, uint32_t(sizeof(vsg::vec2)), VK_VERTEX_INPUT_RATE_VERTEX});
                vertexAttributeDescriptions.push_back({vertexBindingIndex, vertexBindingIndex, VK_FORMAT_R32G32_SFLOAT, 0});
                ++vertexBindingIndex;
            }
        }

        auto pipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

        vsg::ColorBlendState::ColorBlendAttachments colorBlendAttachments;
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.blendEnable = key.blend;
        if (key.colorWrite)
        {
            colorBlendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
            if (key.blend)
                colorBlendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
        }
        if (key.blend)
        {
            colorBlendAttachment.srcColorBlendFactor = key.srcBlendFactor;
            colorBlendAttachment.dstColorBlendFactor = key.dstBlendFactor;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = key.srcAlphaBlendFactor;
            colorBlendAttachment.dstAlphaBlendFactor = key.dstAlphaBlendFactor;;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        colorBlendAttachments.push_back(colorBlendAttachment);

        auto depthStencilState = vsg::DepthStencilState::create();
        depthStencilState->depthTestEnable = key.depthTest;
        depthStencilState->depthWriteEnable = key.depthWrite;

        auto rasterState = vsg::RasterizationState::create();
        rasterState->cullMode = key.cullMode;
        rasterState->frontFace = key.frontFace;
        rasterState->polygonMode = key.polygonMode;

        vsg::GraphicsPipelineStates pipelineStates{
            vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
            vsg::InputAssemblyState::create(key.primitiveTopology),
            rasterState,
            vsg::ColorBlendState::create(colorBlendAttachments),
            vsg::MultisampleState::create(),
            depthStencilState};

        auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(vsg::GraphicsPipeline::create(pipelineLayout, shaders, pipelineStates));
        return bindGraphicsPipeline;
    }
}
