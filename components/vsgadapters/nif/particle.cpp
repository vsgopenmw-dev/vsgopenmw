#include "particle.hpp"

#include <iostream>

#include <vsg/nodes/Switch.h>
#include <vsg/commands/Dispatch.h>

#include <components/animation/bones.hpp>
#include <components/animation/context.hpp>
#include <components/animation/range.hpp>
#include <components/animation/transform.hpp>
#include <components/vsgutil/computetransform.hpp>
#include <components/vsgutil/id.hpp>
#include <components/vsgutil/nodepath.hpp>

#include "anim.hpp"

namespace vsgAdapters
{
    float getParticlesPerSecond(const Nif::NiParticleSystemController& partctrl)
    {
        if (partctrl.noAutoAdjust())
            return partctrl.emitRate;
        else if (partctrl.lifetime == 0 && partctrl.lifetimeRandom == 0)
            return 0.f;
        else
            return (partctrl.numParticles / (partctrl.lifetime + partctrl.lifetimeRandom / 2));
    }

    Pipeline::Data::EmitArgs handleEmitterData(const Nif::NiParticleSystemController& partctrl)
    {
        Pipeline::Data::EmitArgs data{
            .offsetRandom = { toVsg(partctrl.offsetRandom), 0 },
            .direction = { partctrl.horizontalDir, partctrl.horizontalAngle, partctrl.verticalDir, partctrl.verticalAngle },
            .velocityLifetime = { partctrl.velocity, partctrl.velocityRandom, partctrl.lifetime, partctrl.lifetimeRandom }
        };
        return data;
    }

    void handleInitialParticles(vsg::Array<Pipeline::Data::Particle>& array, const Nif::NiParticlesData& data,
        const Nif::NiParticleSystemController& partctrl)
    {
        for (int i = 0; i < data.numParticles; ++i)
        {
            auto& in = partctrl.particles[i];
            float age = std::max(0.f, in.lifetime);
            if (i >= data.activeCount || in.vertex >= data.vertices.size())
                age = in.lifespan;
            auto& out = array.at(i);
            out.velocityAge = vsg::vec4(toVsg(in.velocity), age);
            out.maxAge.x = in.lifespan;
            out.color = toVsg(partctrl.color);
            out.color.a = 1;
            if (in.vertex < data.vertices.size())
                out.positionSize = vsg::vec4(toVsg(data.vertices[in.vertex]), 1.f);
            float size = partctrl.size;
            if (in.vertex < data.sizes.size())
                size *= data.sizes[in.vertex];
            out.positionSize.w = size;
        }
    }

    vsg::ref_ptr<vsg::Data> createColorCurve(const Nif::NiColorData& clrdata, float maxDuration, float timeStep)
    {
        auto channel = handleInterpolatedKeyframes<vsg::vec4>(clrdata.mKeyMap);
        int numSteps = maxDuration / timeStep;
        auto data = vsg::vec4Array::create(numSteps, vsg::Data::Properties(VK_FORMAT_R32G32B32A32_SFLOAT));
        for (int i = 0; i < numSteps; ++i)
            data->at(i) = channel->value(timeStep * i);
        return data;
    }

    class LinkEmitter : public vsg::ConstVisitor
    {
        Emitter& mEmitter;
        vsgUtil::AccumulatePath<const Anim::Transform*> mTransformPath;
        vsgUtil::AccumulatePath<const vsg::Switch*> mSwitchPath;

    public:
        bool foundEmitterNode = false;
        bool foundParticleNode = false;
        LinkEmitter(vsgAdapters::Emitter& e)
            : mEmitter(e)
        {
            overrideMask = vsg::MASK_ALL;
        }
        using vsg::ConstVisitor::apply;
        void apply(const vsg::Switch& sw) override
        {
            auto ppn = mSwitchPath.pushPop(&sw);
            check(sw);
            sw.traverse(*this);
        }
        void apply(const vsg::Transform& t) override
        {
            auto trans = dynamic_cast<const Anim::Transform*>(&t);
            if (!trans)
            {
                check(t);
                t.traverse(*this);
                return;
            }
            auto ppn = mTransformPath.pushPop(trans);
            check(t);
            trans->traverse(*this);
        }
        void apply(const vsg::Node& n) override
        {
            check(n);
            n.traverse(*this);
        }
        void check(const vsg::Node& n)
        {
            if (auto id = vsgUtil::ID::get(n))
            {
                int index = id->id;
                if (index == mEmitter.mEmitterNodeIndex)
                {
                    foundEmitterNode = true;
                    mEmitter.mPathToEmitter = mTransformPath.path;
                    mEmitter.mSwitches = mSwitchPath.path;
                }
                else if (index == mEmitter.mParticleNodeIndex)
                {
                    foundParticleNode = true;
                    mEmitter.mPathToParticle = mTransformPath.path;
                }
            }
        }
    };

    Emitter::Emitter(const Nif::NiParticleSystemController& partctrl, int particleNodeIndex)
        : mParticleNodeIndex(particleNodeIndex)
    {
        mEmitArgs = handleEmitterData(partctrl);
        mParticlesPerSecond = getParticlesPerSecond(partctrl);

        if (partctrl.timeStop <= partctrl.stopTime && partctrl.timeStart >= partctrl.startTime)
            active = Anim::make_constant(true);
        else
        {
            active = Anim::make_channel<Anim::Range>(partctrl.startTime, partctrl.stopTime);
            addExtrapolatorIfRequired(partctrl, active, partctrl.startTime, partctrl.stopTime);
        }
        if (!partctrl.emitter.empty())
            mEmitterNodeIndex = partctrl.emitter->recIndex;
    }

    void Emitter::apply(vsg::Value<Pipeline::Data::FrameArgs>& val, float time)
    {
        // float dt = scene.dt;
        // openmw-7238-particle-system-time
        float dt = mDt.get(time);
        auto& data = val.value();
        data.time.x = dt;
        data.time.y = time;
        if (active->value(time) && emitterVisible())
        {
            data.emitMatrix = calculateEmitMatrix();
            data.emitCount = { calculateEmitCount(dt), 0, 0, 0 };
        }
        else
            data.emitCount = {};
    }

    void Emitter::link(Anim::Context& ctx, vsg::Object&)
    {
        LinkEmitter visitor(*this);
        ctx.attachmentPath.back()->accept(visitor);
        if (mEmitterNodeIndex != -1 && !visitor.foundEmitterNode)
            std::cerr << "!foundEmitterNode(" << mEmitterNodeIndex << ")" << std::endl;
        if (mParticleNodeIndex!= -1 && !visitor.foundParticleNode)
            std::cerr << "!foundParticleNode(" << mParticleNodeIndex << ")" << std::endl;

        vsgUtil::trim(mPathToEmitter, mPathToParticle);
    }

    int Emitter::calculateEmitCount(float dt)
    {
        auto v = dt * mParticlesPerSecond * 0.01;
        int i = static_cast<int>(v);
        mCarryOver += v - static_cast<float>(i);
        if (mCarryOver > 1.f)
        {
            ++i;
            mCarryOver -= 1.f;
        }
        return i;
    }

    vsg::mat4 Emitter::calculateEmitMatrix() const
    {
        auto emitterToWorld = vsgUtil::computeTransform(mPathToEmitter);
        auto worldToPs = vsg::inverse_4x3(vsgUtil::computeTransform(mPathToParticle));
        return /*orthoNormalize(*/ worldToPs * emitterToWorld;
    }

    bool Emitter::emitterVisible() const
    {
        for (auto sw : mSwitches)
            for (auto& switchChild : sw->children)
                if (switchChild.mask == vsg::MASK_OFF)
                    return false;
        return true;
    }

    vsg::ref_ptr<vsg::Command> createParticleDispatch(uint32_t numParticles)
    {
        #include <files/shaders/comp/particle/workgroupsize.glsl>
        // Vulkan guarantees a minimum of 128 maxComputeWorkGroupInvocations.
        static_assert(workGroupSizeX <= 128);
        return vsg::Dispatch::create(std::ceil(numParticles/static_cast<float>(workGroupSizeX)), 1, 1);
    }
}
