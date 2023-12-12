#include "particle.hpp"

#include <components/vsgutil/readshader.hpp>

namespace Pipeline
{
    namespace Descriptors
    {
        #include <files/shaders/lib/material/bindings.glsl>
        #include <files/shaders/comp/particle/bindings.glsl>
    }

    Particle::Particle(vsg::ref_ptr<const vsg::Options> readShaderOptions)
        : mShaderOptions(readShaderOptions)
    {
        auto computeStage = VK_SHADER_STAGE_COMPUTE_BIT;

        vsg::DescriptorSetLayoutBindings descriptorBindings{
            { Descriptors::PARTICLE_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, computeStage, nullptr },
            { Descriptors::SIMULATE_ARGS_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, computeStage, nullptr },
            { Descriptors::FRAME_ARGS_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, computeStage, nullptr },
            { Descriptors::COLOR_CURVE_BINDING, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, computeStage, nullptr }
        };
        static const auto nullDsl = vsg::DescriptorSetLayout::create();
        vsg::DescriptorSetLayouts descriptorSetLayouts = {
            nullDsl,
            nullDsl,
            vsg::DescriptorSetLayout::create(descriptorBindings)
        };

        mLayout = vsg::PipelineLayout::create(descriptorSetLayouts, vsg::PushConstantRanges());
    }

    Particle::~Particle() {}

    vsg::ref_ptr<vsg::BindComputePipeline> Particle::create(ParticleModeFlags modes) const
    {
        auto shaderStage = vsgUtil::readShader("comp/particle/simulate.comp", mShaderOptions);
        shaderStage = vsg::ShaderStage::create(*shaderStage);
        shaderStage->specializationConstants = {
            { 0, vsg::Value<VkBool32>::create(modes & ParticleMode_Size) },
            { 1, vsg::Value<VkBool32>::create(modes & ParticleMode_Color) },
            { 2, vsg::Value<VkBool32>::create(modes & ParticleMode_GravityPlane) },
            { 3, vsg::Value<VkBool32>::create(modes & ParticleMode_GravityPoint) },
            { 4, vsg::Value<VkBool32>::create(modes & ParticleMode_CollidePlane) },
            { 5, vsg::Value<VkBool32>::create(modes & ParticleMode_WorldSpace) }
        };
        return vsg::BindComputePipeline::create(vsg::ComputePipeline::create(mLayout, shaderStage));
    }
}
