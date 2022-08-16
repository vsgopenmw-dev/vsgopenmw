#include "particle.hpp"

#include <vsg/nodes/Switch.h>

#include <components/animation/transform.hpp>
#include <components/animation/range.hpp>
#include <components/animation/bones.hpp>
#include <components/animation/context.hpp>
#include <components/vsgutil/id.hpp>
#include <components/vsgutil/computetransform.hpp>

#include "anim.hpp"

namespace
{
    template <class Path>
    void optimizePaths(Path &p1, Path &p2)
    {
        Path newp1;
        Path newp2;
        newp1.swap(p1);
        newp2.swap(p2);
        auto i1 = newp1.begin();
        auto i2 = newp2.begin();
        while (i1 != newp1.end() && i2 != newp2.end() && *i1 == *i2)
        {
            ++i1;
            ++i2;
        }
        std::copy(i1, newp1.end(), std::back_inserter(p1));
        std::copy(i2, newp2.end(), std::back_inserter(p2));
    }
}

namespace vsgAdapters
{
    float getParticlesPerSecond(const Nif::NiParticleSystemController &partctrl)
    {
        if (partctrl.noAutoAdjust())
            return partctrl.emitRate;
        else if (partctrl.lifetime == 0 && partctrl.lifetimeRandom == 0)
            return 0.f;
        else
            return (partctrl.numParticles / (partctrl.lifetime + partctrl.lifetimeRandom/2));
    }

    Pipeline::Data::EmitArgs handleEmitterData(const Nif::NiParticleSystemController &partctrl)
    {
        Pipeline::Data::EmitArgs data{
            .horizontalDir = partctrl.horizontalDir,
            .horizontalAngle = partctrl.horizontalAngle,
            .verticalDir = partctrl.verticalDir,
            .verticalAngle = partctrl.verticalAngle,
            .velocity = partctrl.velocity,
            .velocityRandom = partctrl.velocityRandom,
            .lifetime = partctrl.lifetime,
            .lifetimeRandom = partctrl.lifetimeRandom,
            .offsetRandom = toVsg(partctrl.offsetRandom)
        };
        return data;
    }

    void handleInitialParticles(vsg::Array<Pipeline::Data::Particle> &array, const Nif::NiParticlesData &data, const Nif::NiParticleSystemController &partctrl)
    {
        for (int i=0; i<data.numParticles; ++i)
        {
            auto &in = partctrl.particles[i];
            float age = std::max(0.f, in.lifetime);
            if (i >= data.activeCount || in.vertex >= data.vertices.size())
                age = in.lifespan;
            auto &out = array.at(i);
            out.velocityAge = vsg::vec4(toVsg(in.velocity), age);
            out.maxAge = in.lifespan;
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

    vsg::ref_ptr<vsg::Data> createColorCurve(const Nif::NiColorData &clrdata, float maxDuration, float timeStep)
    {
        auto channel = handleInterpolatedKeyframes<vsg::vec4>(clrdata.mKeyMap);
        int numSteps = maxDuration / timeStep;
        auto data = vsg::vec4Array::create(numSteps);
        for (int i=0; i<numSteps; ++i)
            data->at(i) = channel->value(timeStep * i);
        return data;
    }

    class LinkEmitter : public vsg::ConstVisitor
    {
        Emitter &mEmitter;
        std::vector<const Anim::Transform*> mTransformPath;
        std::vector<const vsg::Switch*> mSwitchPath;
    public:
        LinkEmitter(vsgAdapters::Emitter &e) : mEmitter(e)
        {
            overrideMask = vsg::MASK_ALL;
        }
        using vsg::ConstVisitor::apply;
        void apply(const vsg::Switch &sw) override
        {
            assert(sw.children.size() == 1);
            mSwitchPath.emplace_back(&sw);
            sw.traverse(*this);
            mSwitchPath.pop_back();
        }
        void apply(const vsg::Transform &t) override
        {
            auto trans = dynamic_cast<const Anim::Transform*>(&t);
            if (!trans)
            {
                t.traverse(*this);
                return;
            }
            mTransformPath.emplace_back(trans);
            check(t);
            trans->traverse(*this);
            mTransformPath.pop_back();
        }
        void apply(const vsg::Node &n) override
        {
            check(n);
            n.traverse(*this);
        }
        void check(const vsg::Node &n)
        {
            if (auto id = vsgUtil::ID::get(n))
            {
                int index = id->id;
                if (index == mEmitter.emitterNodeIndex)
                {
                    mEmitter.mPathToEmitter = mTransformPath;
                    mEmitter.mSwitches = mSwitchPath;
                }
                else if (index == mEmitter.particleNodeIndex)
                    mEmitter.mPathToParticle = mTransformPath;
            }
        }
    };

    void Emitter::setup(const Nif::NiParticleSystemController &partctrl)
    {
        particlesPerSecond = getParticlesPerSecond(partctrl);

            if (partctrl.timeStop <= partctrl.stopTime && partctrl.timeStart >= partctrl.startTime)
                active = new Anim::Constant(true);
            else
            {
                active = vsg::ref_ptr{new Anim::Range(partctrl.startTime, partctrl.stopTime)};
                addExtrapolatorIfRequired(partctrl, active, partctrl.startTime, partctrl.stopTime);
            }
            if (!partctrl.emitter.empty())
                emitterNodeIndex = partctrl.emitter->recIndex;
    }

    void Emitter::apply(vsg::Value<Pipeline::Data::FrameArgs> &val, float time)
    {
        float dt = time - mLastTime; //globalDt
        mLastTime = time;
        auto &data = val.value();
        data.dt = dt;
        for (size_t i=0; i<sizeof(data.random)/sizeof(data.random[0]); ++i)
            data.random[i] = std::rand()/RAND_MAX;
        for (size_t i=0; i<sizeof(data.signedRandom)/sizeof(data.signedRandom[0]); ++i)
            data.signedRandom[i] = std::rand()/RAND_MAX * 2.f - 1.f;
        if (active->value(time) && emitterVisible())
        {
            data.emitMatrix = calculateEmitMatrix();
            data.emitCount = calculateEmitCount(dt);
        }
        else
            data.emitCount = 0;
    }

    void Emitter::link(Anim::Context &ctx, vsg::Object &)
    {
        LinkEmitter visitor(*this);
        ctx.scene->accept(visitor);
        optimizePaths(mPathToEmitter, mPathToParticle);
    }

    int Emitter::calculateEmitCount(float dt)
    {
        auto v = dt*particlesPerSecond*0.01;
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
        return /*orthoNormalize(*/worldToPs * emitterToWorld;
    }

    bool Emitter::emitterVisible() const
    {
        for (auto sw : mSwitches)
            if (sw->children[0].mask == vsg::MASK_OFF)
                return false;
        return true;
    }
}
