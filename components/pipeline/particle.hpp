#ifndef VSGOPENMW_PIPELINE_PARTICLE_H
#define VSGOPENMW_PIPELINE_PARTICLE_H

#include <vsg/state/ComputePipeline.h>

#include <components/vsgutil/cache.hpp>

namespace Pipeline
{
    enum class ParticleStage
    {
        Simulate,
        Color,
        Size
    };

    /*
     * Creates particle compute pipelines.
     */
    class Particle : public vsgUtil::RefCache<ParticleStage, vsg::BindComputePipeline>
    {
        vsg::ref_ptr<const vsg::Options> mShaderOptions;
        vsg::ref_ptr<vsg::PipelineLayout> mLayout;
    public:
        Particle(vsg::ref_ptr<const vsg::Options> readShaderOptions);
        ~Particle();
        vsg::ref_ptr<vsg::BindComputePipeline> create(const ParticleStage &) const;
        vsg::ref_ptr<vsg::BindComputePipeline> getOrCreate(const ParticleStage &stage) const
        {
            return vsgUtil::RefCache<ParticleStage, vsg::BindComputePipeline>::getOrCreate(stage, *this);
        }
        vsg::PipelineLayout *getLayout() { return mLayout; }
    };
}

#endif
