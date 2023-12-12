#ifndef VSGOPENMW_VSGADAPTERS_NIF_PARTICLE_H
#define VSGOPENMW_VSGADAPTERS_NIF_PARTICLE_H

#include <optional>
#include <deque>

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
    /*
     * ParticleSystem is a controller class for updating the parameters used by particle compute shaders.
     */
    class ParticleSystem : public Anim::MUpdateData<ParticleSystem, vsg::Value<Pipeline::Data::FrameArgs>>
    {
        Anim::DeltaTime mDt;
        friend class LinkParticleSystem;
        int mCarryOver{};
        using TransformPath = std::vector<const Anim::Transform*>;
        TransformPath mLocalToWorld;
        std::optional<vsg::mat4> mLastWorldMatrix;
        TransformPath mPathToParticle;
        std::vector<TransformPath> mPathsToEmitters;
        std::vector<const vsg::Switch*> mSwitches;
        bool mWorldSpace = false;

        /*
         * Links to parent Cull/DepthSorted nodes.
         */
        struct WorldBounds
        {
            std::deque<std::pair<float /*time*/, float /*moved*/>> moved;
            float totalMoved = 0;
            float timer = 0;
        } mWorldBound;
        float mMaxLifetime = 0;
        float mEmitRadius = 0;
        std::vector<vsg::dsphere*> mDynamicBounds;
        vsg::dsphere mInitialBound;
        void updateBounds();

        bool mArrayEmitter = false;
        int mEmitterNodeIndex = -1;
        int mParticleNodeIndex = -1;

        float mParticlesPerSecond{};
        int calculateEmitCount(float dt);
        vsg::mat4 calculateEmitMatrix();
        /*
         * Stops emitting particles when the emitter node is switched off.
         */
        bool emitterVisible() const;
    public:
        ParticleSystem(const Nif::NiParticleSystemController& partctrl, int particleNodeIndex, bool worldSpace);
        Anim::channel_ptr<bool> active;

        static float calculateRadius(const Nif::NiParticleSystemController& partctrl);
        static float calculateMaxLifetime(const Nif::NiParticleSystemController& partctrl);

        void apply(vsg::Value<Pipeline::Data::FrameArgs>& val, float time);
        void link(Anim::Context& ctx, vsg::Object&) override;
    };

    Pipeline::Data::EmitArgs handleEmitterData(const Nif::NiParticleSystemController& partctrl);

    void handleInitialParticles(vsg::Array<Pipeline::Data::Particle>& array, const Nif::NiParticlesData& data, const Nif::NiParticleSystemController& partctrl);

    vsg::ref_ptr<vsg::Data> createColorCurve(const Nif::NiColorData& clrdata, float maxDuration, float timeStep);
    vsg::ref_ptr<vsg::Command> createParticleDispatch(uint32_t numParticles);
}

#endif
