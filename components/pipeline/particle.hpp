#ifndef VSGOPENMW_PIPELINE_PARTICLE_H
#define VSGOPENMW_PIPELINE_PARTICLE_H

#include <vsg/state/ComputePipeline.h>

#include <components/vsgutil/cache.hpp>

namespace Pipeline
{
    enum ParticleModeFlagBits : uint32_t
    {
        ParticleMode_Color = 1,
        ParticleMode_Size = 1<<1,
        ParticleMode_GravityPlane = 1<<2,
        ParticleMode_GravityPoint = 1<<3,
        ParticleMode_CollidePlane = 1<<4,
        ParticleMode_CollideSphere = 1<<5,
    };
    using ParticleModeFlags = uint32_t;

    /*
     * Creates particle compute pipelines.
     */
    class Particle
    {
        vsg::ref_ptr<const vsg::Options> mShaderOptions;
        vsg::ref_ptr<vsg::PipelineLayout> mLayout;
        vsgUtil::RefCache<ParticleModeFlags, vsg::BindComputePipeline> mCache;

    public:
        Particle(vsg::ref_ptr<const vsg::Options> readShaderOptions);
        ~Particle();
        vsg::ref_ptr<vsg::BindComputePipeline> create(ParticleModeFlags modes) const;
        vsg::ref_ptr<vsg::BindComputePipeline> getOrCreate(ParticleModeFlags modes) const
        {
            return mCache.getOrCreate(modes, *this);
        }
        vsg::PipelineLayout* getLayout() { return mLayout; }
    };
}

#endif
