#ifndef VSGOPENMW_VSGADAPTERS_NIF_PARTICLE_H
#define VSGOPENMW_VSGADAPTERS_NIF_PARTICLE_H

#include <vsg/core/Array.h>
#include <vsg/core/Value.h>
#include <vsg/commands/Command.h>

#include <components/animation/deltatime.hpp>
#include <components/animation/channel.hpp>
#include <components/animation/updatedata.hpp>
#include <components/pipeline/particledata.hpp>

namespace Nif
{
    class NiParticleSystemController;
    class NiColorData;
    class NiParticlesData;
}
namespace Anim
{
    class Transform;
}
namespace Pipeline::Data
{
    #include <files/shaders/comp/particle/data.glsl>
}
namespace vsgAdapters
{
    class Emitter : public Anim::MUpdateData<Emitter, vsg::Value<Pipeline::Data::FrameArgs>>
    {
        Anim::DeltaTime mDt;
        friend class LinkEmitter;
        int mCarryOver{};
        std::vector<const Anim::Transform*> mPathToParticle;
        std::vector<const Anim::Transform*> mPathToEmitter;
        std::vector<const vsg::Switch*> mSwitches;
        int mEmitterNodeIndex = -1;
        int mParticleNodeIndex = -1;
        float mParticlesPerSecond{};
        int calculateEmitCount(float dt);
        vsg::mat4 calculateEmitMatrix() const;
        bool emitterVisible() const;
        Pipeline::Data::EmitArgs mEmitArgs;
    public:
        Emitter(const Nif::NiParticleSystemController& partctrl, int particleNodeIndex);
        Anim::channel_ptr<bool> active;

        void apply(vsg::Value<Pipeline::Data::FrameArgs>& val, float time);
        void link(Anim::Context& ctx, vsg::Object&) override;
    };

    Pipeline::Data::EmitArgs handleEmitterData(const Nif::NiParticleSystemController& partctrl);

    void handleInitialParticles(vsg::Array<Pipeline::Data::Particle>& array, const Nif::NiParticlesData& data,
        const Nif::NiParticleSystemController& partctrl);

    vsg::ref_ptr<vsg::Data> createColorCurve(const Nif::NiColorData& clrdata, float maxDuration, float timeStep);
    vsg::ref_ptr<vsg::Command> createParticleDispatch(uint32_t numParticles);
}

#endif
