#include "particle.hpp"

#include <components/vsgutil/readshader.hpp>

#include "layout.hpp"
#include "bindings.hpp"
#include "computebindings.hpp"

namespace Pipeline
{
    std::string getFilename(const ParticleStage &stage)
    {
        switch (stage)
        {
            case ParticleStage::Simulate: return "simulate.comp";
            case ParticleStage::Color: return "color.comp";
            case ParticleStage::Size: return "size.comp";
        }
    }

    Particle::Particle(vsg::ref_ptr<const vsg::Options> readShaderOptions) : mShaderOptions(readShaderOptions)
    {
        vsg::PushConstantRanges pushConstantRanges{
            //{VK_SHADER_STAGE_VERTEX_BIT, 0, getCompatiblePushConstantSize()}
        };

        auto computeStage = VK_SHADER_STAGE_COMPUTE_BIT;
        vsg::DescriptorSetLayoutBindings particleDescriptorBindings{
            {Descriptors::PARTICLE_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, computeStage, nullptr}};

        vsg::DescriptorSetLayoutBindings computeDescriptorBindings{
            {Descriptors::STORAGE_ARGS_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1, computeStage, nullptr},
            {Descriptors::ARGS_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, computeStage, nullptr}
        };
        vsg::DescriptorSetLayouts descriptorSetLayouts = getCompatibleDescriptorSetLayouts();
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(particleDescriptorBindings));
        descriptorSetLayouts.emplace_back(vsg::DescriptorSetLayout::create(computeDescriptorBindings));

        mLayout = vsg::PipelineLayout::create(descriptorSetLayouts, vsg::PushConstantRanges());
    }

    Particle::~Particle()
    {
    }

    vsg::ref_ptr<vsg::BindComputePipeline> Particle::create(const ParticleStage &stage) const
    {
        auto shaderStage = vsgUtil::readShader("comp/particle/" + getFilename(stage), mShaderOptions);
        return vsg::BindComputePipeline::create(vsg::ComputePipeline::create(mLayout, shaderStage));
    }
}
