#include "graphics.hpp"

#include <vsg/state/ColorBlendState.h>
#include <vsg/state/DepthStencilState.h>
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/MultisampleState.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/state/VertexInputState.h>

#include <components/vsgutil/readshader.hpp>

#include "bindings.hpp"
#include "layout.hpp"

namespace Pipeline
{
    std::string getDefine(Mode mode)
    {
        switch (mode)
        {
            case Mode::MATERIAL:
                return "MATERIAL";
            case Mode::TEXMAT:
                return "TEXMAT";
            case Mode::DIFFUSE_MAP:
                return "DIFFUSE_MAP";
            case Mode::DARK_MAP:
                return "DARK_MAP";
            case Mode::DETAIL_MAP:
                return "DETAIL_MAP";
            case Mode::DECAL_MAP:
                return "DECAL_MAP";
            case Mode::GLOW_MAP:
                return "GLOW_MAP";
            case Mode::BUMP_MAP:
                return "BUMP_MAP";
            case Mode::ENV_MAP:
                return "ENV_MAP";
            case Mode::BILLBOARD:
                return "BILLBOARD";
            case Mode::PARTICLE:
                return "PARTICLE";
            case Mode::MORPH:
                return "MORPH";
            case Mode::VERTEX:
                return "VERTEX";
            case Mode::NORMAL:
                return "NORMAL";
            case Mode::COLOR:
                return "COLOR";
            case Mode::TEXCOORD:
                return "TEXCOORD";
            case Mode::SKIN:
                return "SKIN";
        }
    }

    Graphics::Graphics(vsg::ref_ptr<const vsg::Options> readShaderOptions)
        : mShaderOptions(readShaderOptions)
    {
        /*
         * A fixed layout may allow the implementation to cheaply switch pipelines without reprogramming the bindings.
         */
        vsg::DescriptorSetLayoutBindings descriptorBindings = {
            { Descriptors::MATERIAL_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { Descriptors::PARTICLE_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr },
            { Descriptors::MORPH_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr },
            { Descriptors::MORPH_WEIGHTS_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr },
            { Descriptors::BONE_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr },
            { Descriptors::DIFFUSE_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { Descriptors::DARK_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { Descriptors::DETAIL_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { Descriptors::DECAL_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { Descriptors::GLOW_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { Descriptors::BUMP_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { Descriptors::ENV_UNIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            { Descriptors::TEXMAT_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr }
        };

        vsg::DescriptorSetLayouts descriptorSetLayouts = getCompatibleDescriptorSetLayouts();
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(descriptorBindings));

        vsg::PushConstantRanges pushConstantRanges{ { VK_SHADER_STAGE_VERTEX_BIT, 0,
            getCompatiblePushConstantSize() } };

        mLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);
    }

    Graphics::~Graphics() {}

    vsg::ref_ptr<vsg::ShaderStage> Graphics::createShaderStage(const ShaderOptions& key, const std::string& suffix) const
    {
        auto compileSettings = vsg::ShaderCompileSettings::create();
        for (size_t i = 0; i < key.modes.size(); ++i)
        {
            if (key.modes[i])
                compileSettings->defines.insert(getDefine(static_cast<Mode>(i)));
        }
        auto path = key.path;
        if (path.empty())
            path = "default";
        auto shaderStage = vsgUtil::readShader(path + suffix, mShaderOptions);
        //if (!compileSettings->defines.empty())
        {
            shaderStage = vsg::ShaderStage::create(*shaderStage);
            shaderStage->module = vsg::ShaderModule::create(*shaderStage->module);
            shaderStage->module->hints = compileSettings;
        }
        return shaderStage;
    }

    vsg::ref_ptr<vsg::ShaderStage> Graphics::create(const FragmentStageOptions& key) const
    {
        return createShaderStage(key, "/main.frag");
    }

    vsg::ref_ptr<vsg::ShaderStage> Graphics::create(const VertexStageOptions& key) const
    {
        return createShaderStage(key, "/main.vert");
    }

    vsg::ref_ptr<vsg::BindGraphicsPipeline> Graphics::create(const Options& key) const
    {
        VertexStageOptions vertexStageOptions{ key.shader.path, {} };
        for (auto mode : vertexModes)
        {
            if (key.shader.hasMode(mode))
                vertexStageOptions.addMode(mode);
        }
        auto vertexStage = mVertexStageCache.getOrCreate(vertexStageOptions, *this);

        FragmentStageOptions fragmentStageOptions{ key.shader.path, {} };
        for (auto mode : fragmentModes)
        {
            if (key.shader.hasMode(mode))
                fragmentStageOptions.addMode(mode);
        }
        auto fragmentStage = mFragmentStageCache.getOrCreate(fragmentStageOptions, *this);

        vsg::ShaderStages shaders = { vsg::ShaderStage::create(*vertexStage), vsg::ShaderStage::create(*fragmentStage) };

        auto numUvSets = key.numUvSets;
        vsg::ShaderStage::SpecializationConstants specConstants = { { 0, vsg::intValue::create(numUvSets) } };

        auto vertexSpecConstants = specConstants;
        for (auto [binding, uvSet] : key.nonStandardUvSets)
            if (binding == Descriptors::TEXMAT_BINDING)
                vertexSpecConstants.emplace(Descriptors::START_UVSET_CONSTANTS + binding, vsg::intValue::create(uvSet));

        shaders[0]->specializationConstants = vertexSpecConstants;

        auto fragmentSpecConstants = specConstants;
        fragmentSpecConstants.emplace(1, vsg::intValue::create(key.alphaTestMode));
        fragmentSpecConstants.emplace(2, vsg::intValue::create(key.colorMode));
        fragmentSpecConstants.emplace(3, vsg::Value<VkBool32>::create(key.specular));
        for (auto [binding, uvSet] : key.nonStandardUvSets)
            if (binding != Descriptors::TEXMAT_BINDING && binding != Descriptors::ENV_UNIT)
                fragmentSpecConstants.emplace(
                    Descriptors::START_UVSET_CONSTANTS + binding, vsg::intValue::create(uvSet));
        shaders[1]->specializationConstants = fragmentSpecConstants;

        uint32_t vertexBindingIndex = 0;
        vsg::VertexInputState::Bindings vertexBindingsDescriptions;
        vsg::VertexInputState::Attributes vertexAttributeDescriptions;
        static_assert(static_cast<int>(Mode::VERTEX) == 0);
        if (key.shader.hasMode(Mode::VERTEX))
        {
            vertexBindingsDescriptions.push_back(
                { vertexBindingIndex, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX });
            vertexAttributeDescriptions.push_back(
                { vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        }
        static_assert(static_cast<int>(Mode::NORMAL) == 1);
        if (key.shader.hasMode(Mode::NORMAL))
        {
            vertexBindingsDescriptions.push_back(
                { vertexBindingIndex, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX });
            vertexAttributeDescriptions.push_back(
                { vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        }
        static_assert(static_cast<int>(Mode::COLOR) == 2);
        if (key.shader.hasMode(Mode::COLOR))
        {
            vertexBindingsDescriptions.push_back(
                { vertexBindingIndex, sizeof(vsg::vec4), VK_VERTEX_INPUT_RATE_VERTEX });
            vertexAttributeDescriptions.push_back(
                { vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32A32_SFLOAT, 0 });
        }
        static_assert(static_cast<int>(Mode::SKIN) == 3);
        if (key.shader.hasMode(Mode::SKIN))
        {
            vertexBindingsDescriptions.push_back(
                { vertexBindingIndex, sizeof(vsg::vec4), VK_VERTEX_INPUT_RATE_VERTEX });
            vertexAttributeDescriptions.push_back(
                { vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32A32_SFLOAT, 0 });

            vertexBindingsDescriptions.push_back(
                { vertexBindingIndex, sizeof(vsg::vec4), VK_VERTEX_INPUT_RATE_VERTEX });
            vertexAttributeDescriptions.push_back(
                { vertexBindingIndex, vertexBindingIndex++, VK_FORMAT_R32G32B32A32_SFLOAT, 0 });
        }
        static_assert(static_cast<int>(Mode::TEXCOORD) == 4);
        if (key.shader.hasMode(Mode::TEXCOORD))
        {
            for (uint32_t i = 0; i < numUvSets; ++i)
            {
                vertexBindingsDescriptions.push_back(
                    { vertexBindingIndex, uint32_t(sizeof(vsg::vec2)), VK_VERTEX_INPUT_RATE_VERTEX });
                vertexAttributeDescriptions.push_back(
                    { vertexBindingIndex, vertexBindingIndex, VK_FORMAT_R32G32_SFLOAT, 0 });
                ++vertexBindingIndex;
            }
        }

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.blendEnable = key.blend;
        if (key.colorWrite)
        {
            colorBlendAttachment.colorWriteMask
                = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }
        if (key.blend)
        {
            colorBlendAttachment.srcColorBlendFactor = key.srcBlendFactor;
            colorBlendAttachment.dstColorBlendFactor = key.dstBlendFactor;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = key.srcAlphaBlendFactor;
            colorBlendAttachment.dstAlphaBlendFactor = key.dstAlphaBlendFactor;
            ;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        vsg::ColorBlendState::ColorBlendAttachments colorBlendAttachments = { colorBlendAttachment };

        auto depthStencilState = vsg::DepthStencilState::create();
        depthStencilState->depthTestEnable = key.depthTest;
        depthStencilState->depthWriteEnable = key.depthWrite;

        auto rasterState = vsg::RasterizationState::create();
        rasterState->cullMode = key.cullMode;
        rasterState->frontFace = key.frontFace;
        rasterState->polygonMode = key.polygonMode;
        if (key.depthBias)
        {
            rasterState->depthBiasEnable = true;
            rasterState->depthBiasConstantFactor = 10000;
            rasterState->depthBiasSlopeFactor = 10000;
        }


        vsg::GraphicsPipelineStates pipelineStates{ vsg::VertexInputState::create(
                                                        vertexBindingsDescriptions, vertexAttributeDescriptions),
            vsg::InputAssemblyState::create(key.primitiveTopology), rasterState,
            vsg::ColorBlendState::create(colorBlendAttachments), vsg::MultisampleState::create(), depthStencilState };

        return vsg::BindGraphicsPipeline::create(vsg::GraphicsPipeline::create(mLayout, shaders, pipelineStates));
    }
}
