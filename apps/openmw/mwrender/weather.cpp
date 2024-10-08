#include "weather.hpp"

#include <vsg/commands/BeginQuery.h>
#include <vsg/commands/Draw.h>
#include <vsg/commands/EndQuery.h>
#include <vsg/commands/ResetQueryPool.h>
#include <vsg/io/Options.h>
#include <vsg/nodes/DepthSorted.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/StateSwitch.h>
#include <vsg/utils/SharedObjects.h>

#include <components/animation/clone.hpp>
#include <components/animation/transform.hpp>
#include <components/fallback/fallback.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>
#include <components/vfs/manager.hpp>
#include <components/pipeline/builder.hpp>
#include <components/pipeline/graphics.hpp>
#include <components/pipeline/material.hpp>
#include <components/pipeline/override.hpp>
#include <components/pipeline/sets.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/settings.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/vsgutil/compilecontext.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/readnode.hpp>
#include <components/vsgutil/readshader.hpp>
#include <components/vsgutil/searchbytype.hpp>
#include <components/vsgutil/setbin.hpp>
#include <components/vsgutil/share.hpp>
#include <components/vsgutil/viewrelative.hpp>

#include "../mwworld/weather.hpp"

#include "bin.hpp"
#include "weatherdata.hpp"

namespace
{
    void depthOff(Pipeline::Options& o)
    {
        o.depthTest = false;
        o.depthWrite = false;
    }

    template <class T>
    void share(vsg::ref_ptr<T>& obj, vsg::ref_ptr<const vsg::Options> options)
    {
        vsgUtil::share_if(options->sharedObjects, obj);
    }

    static const auto sDrawQuad = vsg::Draw::create(4, 1, 0, 0);
}

namespace MWRender
{
    struct Atmosphere
    {
        static void pipelineOptions(Pipeline::Options& o)
        {
            o.shader.path = "sky/atmosphere";
            depthOff(o);
        }
    };

    class AtmosphereNight : public Anim::Transform // vsgUtil::Composite<Anim::Transform> // ToggleComposite
    {
    public:
        Pipeline::Material material;
        static void pipelineOptions(Pipeline::Options& o)
        {
            o.shader.path = "sky/night";
            depthOff(o);
            o.blend = true;
        }
    };

    class Clouds : public Anim::Transform
    {
        vsg::ref_ptr<vsg::StateSwitch> mStateSwitch;
        vsg::ref_ptr<const vsg::BindDescriptorSet> mReplaceBds;
        std::map<vsg::ref_ptr<vsg::Descriptor>, vsg::ref_ptr<vsg::BindDescriptorSet>> mTextureMap;

    public:
        Pipeline::Material material;
        Pipeline::DynamicDescriptorValue<vsg::mat4> texmat{ Pipeline::Descriptors::TEXMAT_BINDING };
        Clouds(vsg::ref_ptr<vsg::Node> node)
        {
            mStateSwitch = vsg::StateSwitch::create();

            auto replaceBds = [this](const vsg::Node& n) -> vsg::ref_ptr<vsg::Node> {
                if (auto* bds = dynamic_cast<const vsg::BindDescriptorSet*>(&n))
                {
                    if (bds->pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS)
                        return {};
                    mReplaceBds = bds;
                    mStateSwitch->slot = bds->slot;
                    return mStateSwitch;
                }
                return {};
            };
            Anim::cloneIfRequired(node, replaceBds);
            addChild(node);
        }
        static void pipelineOptions(Pipeline::Options& o)
        {
            depthOff(o);
            o.blend = true;
            o.shader.path = "sky/clouds";
            o.shader.addMode(Pipeline::Mode::DIFFUSE_MAP);
            o.shader.addMode(Pipeline::Mode::TEXMAT);
            o.shader.addMode(Pipeline::Mode::MATERIAL);
        }
        bool textureValid() const { return !mStateSwitch->children.empty(); }
        void setTexture(vsg::ref_ptr<vsg::Descriptor> texture, vsgUtil::CompileContext& compile)
        {
            if (!mReplaceBds)
                return;
            auto found = mTextureMap.find(texture);
            if (found == mTextureMap.end())
            {
                auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, mReplaceBds->layout,
                    mReplaceBds->firstSet, vsg::Descriptors{ texture, material.descriptor(), texmat.descriptor() });
                if (compile.compile(bds))
                    found = mTextureMap.emplace(texture, bds).first;
            }
            if (found != mTextureMap.end())
                mStateSwitch->children = {{ vsg::MASK_ALL, found->second }};
        }
    };

    class CelestialBody : public Anim::Transform
    {
    public:
        Pipeline::Material material;
        CelestialBody(float scaleFactor = 1.f) { scale = vsg::vec3(450, 450, 450) * scaleFactor; }
        Pipeline::Options pipelineOptions(const std::string& shader)
        {
            auto options = Pipeline::Options{ .shader = { shader, Pipeline::modes({ Pipeline::Mode::MATERIAL, Pipeline::Mode::DIFFUSE_MAP }) },
                .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                .blend = true,
                .cullMode = VK_CULL_MODE_NONE };
            depthOff(options);
            return options;
        }

        void setDirection(vsg::vec3 dir)
        {
            dir = vsg::normalize(dir);
            const float distance = 1000.0f;
            translation = dir * distance;
            setAttitude(vsg::quat(vsg::vec3(0.0f, 0.0f, 1.0f), dir));
        }
    };

    class SunGlare : public vsgUtil::Composite<vsg::StateGroup>
    {
        float mFaderMax;
    public:
        Pipeline::Material material;
        SunGlare(const Pipeline::Builder& builder)
        {
            mNode = vsg::StateGroup::create();
            auto color = toVsg(Fallback::Map::getColour("Weather_Sun_Glare_Fader_Color"));
            // Replicating a design flaw in MW. The color was being set on both ambient and emissive properties, which
            // multiplies the result by two, then finally gets clamped by the fixed function pipeline. With the default
            // INI settings, only the red component gets clamped, so the resulting color looks more orange than red.
            color *= 2;
            for (int i = 0; i < 3; ++i)
                color[i] = std::min(1.f, color[i]);
            mFaderMax = Fallback::Map::getFloat("Weather_Sun_Glare_Fader_Max");
            auto faderAngleMax = Fallback::Map::getFloat("Weather_Sun_Glare_Fader_Angle_Max");
            /*material.faderAngleMax*/ color.a = vsg::radians(faderAngleMax);
            material.value().emissive = color;

            auto pipelineOptions
                = Pipeline::Options{ .shader = { "sky/sunglare", Pipeline::modes({ Pipeline::Mode::MATERIAL }) },
                    .blend = true, .cullMode = VK_CULL_MODE_NONE };
            depthOff(pipelineOptions);

            auto pipeline = builder.graphics->getOrCreate(pipelineOptions);
            auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline->layout,
                Pipeline::TEXTURE_SET, vsg::Descriptors{ material.descriptor() });
            mNode->stateCommands = { pipeline, bds };
            mNode->children = { sDrawQuad };
        }
        void update(float alpha) { material.setAlpha(mFaderMax * alpha); }
    };

    class Sun : public CelestialBody
    {
        Pipeline::Material sunFlashMaterial;
        vsg::ref_ptr<vsg::QueryPool> mQueryPool;
        const uint32_t mQueryCount = 2;
        vsg::ref_ptr<Anim::Transform> mFlash;
        vsg::ref_ptr<vsg::Switch> mSwitch;
        std::unique_ptr<SunGlare> mGlare;
        vsg::ref_ptr<vsg::Node> mResetQueryPool;
        bool mPendingQuery = false;

    public:
        float glareView = 1.f;
        float glareTimeOfDayFade = 1.f;
        float visibleRatio = 0.f;
        bool enabled = true;
        Sun(vsg::ref_ptr<const vsg::Options> textureOptions, const Pipeline::Builder& builder, bool occlusionQueryPrecise)
        {
            VkQueryControlFlags queryControlFlags{};
            if (occlusionQueryPrecise)
                queryControlFlags |= VK_QUERY_CONTROL_PRECISE_BIT;
            {
                mQueryPool = vsg::QueryPool::create();
                mQueryPool->queryType = VK_QUERY_TYPE_OCCLUSION;
                mQueryPool->queryCount = mQueryCount;
                addChild(vsg::BeginQuery::create(mQueryPool, 0, queryControlFlags));
                mResetQueryPool = vsg::ResetQueryPool::create(mQueryPool);
            }

            auto sampler = builder.createSampler();
            sampler->addressModeU = sampler->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            share(sampler, textureOptions);

            auto sunTex = vsg::DescriptorImage::create(sampler, vsgUtil::readImage("tx_sun_05.dds", textureOptions));

            auto pipeline = builder.graphics->getOrCreate(pipelineOptions("sky/sun"));
            auto sunBds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline->layout,
                Pipeline::TEXTURE_SET, vsg::Descriptors{ sunTex, material.descriptor() });
            {
                auto sg = vsg::StateGroup::create();
                sg->stateCommands = { pipeline, sunBds };
                sg->children = { sDrawQuad };
                addChild(sg);
            }
            addChild(vsg::EndQuery::create(mQueryPool, 0));

            auto binGroup = vsg::Group::create();
            binGroup->addChild(vsg::BeginQuery::create(mQueryPool, 1, queryControlFlags));
            {
                auto queryPipeline = builder.graphics->getOrCreate(queryOptions());
                auto sg = vsg::StateGroup::create();
                sg->stateCommands = { queryPipeline, sunBds };
                sg->children = { sDrawQuad };
                binGroup->addChild(sg);
            }
            binGroup->addChild(vsg::EndQuery::create(mQueryPool, 1));

            mSwitch = vsg::Switch::create();
            {
                auto sg = vsg::StateGroup::create();
                auto flashTex = vsgUtil::readImage("tx_sun_flash_grey_05.dds", textureOptions);
                auto sampler = builder.createSampler();
                sampler->addressModeU = sampler->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                share(sampler, textureOptions);
                auto descriptorImage = vsg::DescriptorImage::create(sampler, flashTex);

                sunFlashMaterial.value().emissive = vsg::vec4(1, 1, 1, 1 );
                auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline->layout,
                    Pipeline::TEXTURE_SET, vsg::Descriptors{ descriptorImage, sunFlashMaterial.descriptor() });
                sg->stateCommands = { pipeline, bds };
                mFlash = vsg::ref_ptr{ new Anim::Transform };
                mFlash->children = { sDrawQuad };
                sg->children = { mFlash };
                mSwitch->addChild(false, sg);
            }
            mGlare = std::make_unique<SunGlare>(builder);
            mSwitch->addChild(false, mGlare->node());
            binGroup->addChild(mSwitch);

            addChild(vsgUtil::createBinSetter(Bin_SunGlare, binGroup));
        }
        Pipeline::Options queryOptions()
        {
            auto options = pipelineOptions("sky/sun");
            options.colorWrite = false;
            options.depthTest = true;
            return options;
        }
        vsg::ref_ptr<vsg::Node> resetNode() { return mResetQueryPool; }
        void setColor(const vsg::vec4& color) { material.update(material.value().emissive, color); }
        void updateQuery()
        {
            std::vector<uint64_t> results(mQueryCount);
            if (mQueryPool->getResults(results, 0, 0) != VK_SUCCESS)
                return;
            auto total = results[0];
            auto visible = results[1];
            visibleRatio = total > 0 ? static_cast<float>(visible) / total : 0.f;
        }
        void updateFlash()
        {
            if (visibleRatio > 0.f)
            {
                float scale = 2.6;
                const float fadeThreshold = 0.1;
                if (visibleRatio < fadeThreshold)
                {
                    float fade = 1.f - (fadeThreshold - visibleRatio) / fadeThreshold;
                    sunFlashMaterial.setAlpha(fade);
                }
                else if (visibleRatio < 1.f)
                {
                    sunFlashMaterial.setAlpha(1);
                    const float threshold = 0.6;
                    scale *= visibleRatio * (1.f - threshold) + threshold;
                }
                mFlash->setScale(scale);
            }
        }
        void update()
        {
            if (!enabled)
            {
                mPendingQuery = false;
                return;
            }
            if (mPendingQuery)
                updateQuery();
            mPendingQuery = true;

            auto glare = glareView * visibleRatio;
            mSwitch->setAllChildren(glare > 0.f);

            mGlare->update(glare * glareTimeOfDayFade);
            updateFlash();
        }
    };

    class Moon : public CelestialBody
    {
    public:
        enum class Type
        {
            Masser = 0,
            Secunda
        };

        Moon(vsg::ref_ptr<const vsg::Options> options, float scaleFactor, Type type, const Pipeline::Builder& builder)
            : CelestialBody(scaleFactor)
            , mType(type)
        {
            auto circleImage = vsgUtil::readImage(
                std::string("tx_mooncircle_full_") + (type == Type::Masser ? "m" : "s") + ".dds", options);
            auto sampler = builder.createSampler();
            sampler->addressModeU = sampler->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            share(sampler, options);

            mCircleTex = vsg::DescriptorImage::create(sampler, circleImage);

            auto po = pipelineOptions("sky/moon");
            po.shader.addMode(Pipeline::Mode::GLOW_MAP);
            auto sg = vsg::StateGroup::create();
            mPipeline = builder.graphics->getOrCreate(po);
            mStateSwitch = vsg::StateSwitch::create();
            for (uint32_t i = 0; i < static_cast<uint32_t>(MoonState::Phase::Count); ++i)
            {
                auto bds = createBindDescriptorSet(options, static_cast<MoonState::Phase>(i), builder);
                mStateSwitch->add(vsg::boolToMask(i == 0), bds);
                mStateSwitch->slot = bds->slot;
            }
            sg->stateCommands = { mPipeline, mStateSwitch };
            sg->children = { sDrawQuad };
            children = { sg };
        }
        std::string phaseName(MoonState::Phase phase)
        {
            switch (phase)
            {
                case MoonState::Phase::New:
                    return "new";
                case MoonState::Phase::WaxingCrescent:
                    return "one_wax";
                case MoonState::Phase::FirstQuarter:
                    return "half_wax";
                case MoonState::Phase::WaxingGibbous:
                    return "three_wax";
                case MoonState::Phase::WaningCrescent:
                    return "one_wan";
                case MoonState::Phase::ThirdQuarter:
                    return "half_wan";
                case MoonState::Phase::WaningGibbous:
                    return "three_wan";
                case MoonState::Phase::Full:
                    return "full";
                default:
                    return {};
            }
        }
        vsg::ref_ptr<vsg::BindDescriptorSet> createBindDescriptorSet(
            vsg::ref_ptr<const vsg::Options> options, MoonState::Phase phase, const Pipeline::Builder& builder)
        {
            std::string textureName = "tx_";
            if (mType == Moon::Type::Secunda)
                textureName += "secunda_";
            else
                textureName += "masser_";
            textureName += phaseName(phase) + ".dds";

            auto sampler = builder.createSampler();
            sampler->addressModeU = sampler->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            share(sampler, options);
            auto descriptorImage = vsg::DescriptorImage::create(
                sampler, vsgUtil::readImage(textureName, options), Pipeline::Descriptors::GLOW_UNIT);

            auto descriptors = vsg::Descriptors{ descriptorImage, mCircleTex, material.descriptor() };
            return vsg::BindDescriptorSet::create(
                VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->pipeline->layout, Pipeline::TEXTURE_SET, descriptors);
        }
        void setColor(const vsg::vec4& color)
        {
            material.value().emissive = vsg::vec4(color.x, color.y, color.z, material.value().emissive.a);
        }
        void setState(const MoonState state)
        {
            float radsX = ((state.mRotationFromHorizon) * static_cast<float>(osg::PI)) / 180.0f;
            float radsZ = ((state.mRotationFromNorth) * static_cast<float>(osg::PI)) / 180.0f;

            vsg::quat rotX(radsX, vsg::vec3(1.0f, 0.0f, 0.0f));
            vsg::quat rotZ(radsZ, vsg::vec3(0.0f, 0.0f, 1.0f));

            auto direction = rotX * rotZ * vsg::vec3(0.0f, 1.0f, 0.0f);
            setDirection(direction);

            // The moon quad is initially oriented facing down, so we need to offset its X-axis
            // rotation to rotate it to face the camera when sitting at the horizon.
            vsg::quat attX((-static_cast<float>(vsg::PI) / 2.0f) + radsX, vsg::vec3(1.0f, 0.0f, 0.0f));
            setAttitude(attX * rotZ);

            setPhase(state.mPhase);

            material.update(material.value().emissive.a, state.mShadowBlend);
            material.setAlpha(state.mMoonAlpha);
        }
        unsigned int getPhaseInt() const
        {
            switch (mPhase)
            {
                case MoonState::Phase::New:
                    return 0;
                case MoonState::Phase::WaxingCrescent:
                    return 1;
                case MoonState::Phase::WaningCrescent:
                    return 1;
                case MoonState::Phase::FirstQuarter:
                    return 2;
                case MoonState::Phase::ThirdQuarter:
                    return 2;
                case MoonState::Phase::WaxingGibbous:
                    return 3;
                case MoonState::Phase::WaningGibbous:
                    return 3;
                case MoonState::Phase::Full:
                    return 4;
                default:
                    return 0;
            }
        }
        void setPhase(const MoonState::Phase& phase)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(MoonState::Phase::Count); ++i)
                mStateSwitch->children[i].mask = vsg::boolToMask(static_cast<MoonState::Phase>(i) == phase);
        }

    private:
        vsg::ref_ptr<vsg::DescriptorImage> mCircleTex;
        vsg::ref_ptr<vsg::StateSwitch> mStateSwitch;
        vsg::ref_ptr<vsg::BindGraphicsPipeline> mPipeline;
        Type mType;
        MoonState::Phase mPhase;
    };

    Sky::Sky(Resource::ResourceSystem& resourceSystem, vsg::ref_ptr<vsgUtil::CompileContext> compile, bool occlusionQueryPrecise)
        : mCompile(compile)
        , mTextureOptions(resourceSystem.textureOptions)
        , mStormParticleDirection(MWWorld::Weather::defaultDirection())
        , mStormDirection(MWWorld::Weather::defaultDirection())
    {
        mNode = new vsgUtil::ViewRelative;
        auto nodeOptions = vsg::Options::create(*resourceSystem.nodeOptions);
        auto override_ = vsg::ref_ptr{ new Pipeline::Override };
        override_->attachTo(*nodeOptions);
        mAtmosphere = std::make_unique<Atmosphere>();
        {
            override_->pipelineOptions = Atmosphere::pipelineOptions;
            mNode->addChild(vsgUtil::readNode(Settings::Manager::getString("skyatmosphere", "Models"), nodeOptions));
        }
        override_->material = {};
        mSwitch = vsg::Switch::create();
        mNode->addChild(mSwitch);
        {
            override_->pipelineOptions = AtmosphereNight::pipelineOptions;
            auto t = vsg::ref_ptr{ new AtmosphereNight };
            override_->material = t->material.descriptor();
            vsg::ref_ptr<vsg::Node> node;
            auto night02 = Settings::Manager::getString("skynight02", "Models");
            if (resourceSystem.getVFS()->exists("meshes/" + night02))
                node = vsgUtil::readNode(night02, nodeOptions);
            else
                node = vsgUtil::readNode(Settings::Manager::getString("skynight01", "Models"), nodeOptions);
            t->children = { node };
            mAtmosphereNight.setup(t, mSwitch, false);
            override_->material = {};
        }
        mSun.setup(vsg::ref_ptr{ new Sun(resourceSystem.textureOptions, *resourceSystem.builder, occlusionQueryPrecise) }, mSwitch, true);

        mMasser.setup(
            vsg::ref_ptr{ new Moon(resourceSystem.textureOptions, Fallback::Map::getFloat("Moons_Masser_Size") / 125,
                Moon::Type::Masser, *resourceSystem.builder) },
            mSwitch, true);
        mSecunda.setup(
            vsg::ref_ptr{ new Moon(resourceSystem.textureOptions, Fallback::Map::getFloat("Moons_Secunda_Size") / 125,
                Moon::Type::Secunda, *resourceSystem.builder) },
            mSwitch, true);
        mMoonScriptColor = toVsg(Fallback::Map::getColour("Moons_Script_Color"));

        {
            override_->pipelineOptions = Clouds::pipelineOptions;
            auto node = vsgUtil::readNode(Settings::Manager::getString("skyclouds", "Models"), nodeOptions);
            mClouds.setup(vsg::ref_ptr{ new Clouds(node) }, mSwitch, false);
            mNextClouds.setup(vsg::ref_ptr{ new Clouds(node) }, mSwitch, false);
            // shareDescriptorSet();
        }
        // mUnderwaterSwitch = new UnderwaterSwitchCallback(skyroot);
    }

    Sky::~Sky() {}

    vsg::ref_ptr<vsg::Node> Sky::resetNode()
    {
        return mSun->resetNode();
    }

    void Sky::createRain()
    {
        /*
        if (mRainNode)
            return;

        mRainNode = new osg::Group;

        mRainParticleSystem = new NifOsg::ParticleSystem;
        osg::Vec3 rainRange = osg::Vec3(mRainDiameter, mRainDiameter, (mRainMinHeight+mRainMaxHeight)/2.f);

        mRainParticleSystem->setParticleAlignment(osgParticle::ParticleSystem::FIXED);
        mRainParticleSystem->setAlignVectorX(osg::Vec3f(0.1,0,0));
        mRainParticleSystem->setAlignVectorY(osg::Vec3f(0,0,1));

        osg::ref_ptr<osg::StateSet> stateset = mRainParticleSystem->getOrCreateStateSet();

        osg::ref_ptr<osg::Texture2D> raindropTex = new
        osg::Texture2D(mSceneManager->getImageManager()->getImage("textures/tx_raindrop_01.dds"));
        raindropTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        raindropTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        stateset->setTextureAttributeAndModes(0, raindropTex);
        stateset->setNestRenderBins(false);
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);

        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
        mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
        stateset->setAttributeAndModes(mat);

        osgParticle::Particle& particleTemplate = mRainParticleSystem->getDefaultParticleTemplate();
        particleTemplate.setSizeRange(osgParticle::rangef(5.f, 15.f));
        particleTemplate.setAlphaRange(osgParticle::rangef(1.f, 1.f));
        particleTemplate.setLifeTime(1);

        osg::ref_ptr<osgParticle::ModularEmitter> emitter = new osgParticle::ModularEmitter;
        emitter->setParticleSystem(mRainParticleSystem);

        osg::ref_ptr<osgParticle::BoxPlacer> placer = new osgParticle::BoxPlacer;
        placer->setXRange(-rainRange.x() / 2, rainRange.x() / 2);
        placer->setYRange(-rainRange.y() / 2, rainRange.y() / 2);
        placer->setZRange(-rainRange.z() / 2, rainRange.z() / 2);
        emitter->setPlacer(placer);
        mPlacer = placer;

        // FIXME: vanilla engine does not use a particle system to handle rain, it uses a NIF-file with 20 raindrops in
        it.
        // It spawns the (maxRaindrops-getParticleSystem()->numParticles())*dt/rainEntranceSpeed batches every frame
        (near 1-2).
        // Since the rain is a regular geometry, it produces water ripples, also in theory it can be removed if collides
        with something. osg::ref_ptr<RainCounter> counter = new RainCounter;
        counter->setNumberOfParticlesPerSecondToCreate(mRainMaxRaindrops/mRainEntranceSpeed*20);
        emitter->setCounter(counter);
        mCounter = counter;

        osg::ref_ptr<RainShooter> shooter = new RainShooter;
        mRainShooter = shooter;
        emitter->setShooter(shooter);

        osg::ref_ptr<osgParticle::ParticleSystemUpdater> updater = new osgParticle::ParticleSystemUpdater;
        updater->addParticleSystem(mRainParticleSystem);

        osg::ref_ptr<osgParticle::ModularProgram> program = new osgParticle::ModularProgram;
        program->addOperator(new WrapAroundOperator(mCamera,rainRange));
        program->addOperator(new WeatherAlphaOperator(mPrecipitationAlpha, true));
        program->setParticleSystem(mRainParticleSystem);
        mRainNode->addChild(program);

        mRainNode->addChild(emitter);
        mRainNode->addChild(mRainParticleSystem);
        mRainNode->addChild(updater);

        // Note: if we ever switch to regular geometry rain, it'll need to use an AlphaFader.
        mRainNode->addCullCallback(mUnderwaterSwitch);
        mRainNode->setNodeMask(Mask_WeatherParticles);

        mRainParticleSystem->setUserValue("simpleLighting", true);
        mSceneManager->recreateShaders(mRainNode);

        mRootNode->addChild(mRainNode);
        */
    }

    void Sky::destroyRain()
    {
        /*
        if (!mRainNode)
            return;

        mRootNode->removeChild(mRainNode);
        mRainNode = nullptr;
        mPlacer = nullptr;
        mCounter = nullptr;
        mRainParticleSystem = nullptr;
        mRainShooter = nullptr;
        */
    }

    int Sky::getMasserPhase() const
    {
        return mMasser->getPhaseInt();
    }

    int Sky::getSecundaPhase() const
    {
        return mSecunda->getPhaseInt();
    }

    bool Sky::hasRain() const
    {
        return false; // mRainNode != nullptr;
    }

    float Sky::getRainIntensity() const
    {
        if (!mIsStorm && (hasRain() /*|| mParticleNode*/))
            return mPrecipitationAlpha;
        return 0.f;
    }

    void Sky::update(float duration)
    {
        switchUnderwaterRain();

        /*
        if (mIsStorm && mParticleNode)
        {
            osg::Quat quat;
            quat.makeRotate(MWWorld::Weather::defaultDirection(), mStormParticleDirection);
            // Morrowind deliberately rotates the blizzard mesh, so so should we.
            if (mCurrentParticleEffect == Settings::Manager::getString("weatherblizzard", "Models"))
                quat.makeRotate(osg::Vec3f(-1,0,0), mStormParticleDirection);
            mParticleNode->setAttitude(quat);
        }
        */

        // UV Scroll the clouds
        mCloudAnimationTimer += duration * mCloudSpeed * 0.003;
        vsg::mat4 texmat = vsg::translate(vsg::vec3(0, -mCloudAnimationTimer, 0));
        mClouds->texmat.value() = texmat;
        mClouds->texmat.dirty();
        if (mCloudBlendFactor > 0)
        {
            mNextClouds->texmat.value() = texmat;
            mNextClouds->texmat.dirty();
        }

        // morrowind rotates each cloud mesh independently
        osg::Quat rotation;
        rotation.makeRotate(MWWorld::Weather::defaultDirection(), mStormDirection);
        mClouds->setAttitude(toVsg(rotation));

        rotation.makeRotate(MWWorld::Weather::defaultDirection(), mNextStormDirection);
        mNextClouds->setAttitude(toVsg(rotation));

        mSun->update();
    }

    void Sky::setAtmosphereNightRoll(float roll)
    {
        mAtmosphereNight->setAttitude(vsg::quat(roll, vsg::vec3(0, 0, 1)));
    }

    void Sky::setMoonColour(bool red)
    {
        mSecunda->setColor(red ? mMoonScriptColor : vsg::vec4(1, 1, 1, 1));
    }

    void Sky::updateRainParameters()
    {
        /*
        if (mRainShooter)
        {
            float angle = -std::atan(mWindSpeed/50.f);
            mRainShooter->setVelocity(osg::Vec3f(0, mRainSpeed*std::sin(angle), -mRainSpeed/std::cos(angle)));
            mRainShooter->setAngle(angle);

            osg::Vec3 rainRange = osg::Vec3(mRainDiameter, mRainDiameter, (mRainMinHeight+mRainMaxHeight)/2.f);

            mPlacer->setXRange(-rainRange.x() / 2, rainRange.x() / 2);
            mPlacer->setYRange(-rainRange.y() / 2, rainRange.y() / 2);
            mPlacer->setZRange(-rainRange.z() / 2, rainRange.z() / 2);

            mCounter->setNumberOfParticlesPerSecondToCreate(mRainMaxRaindrops/mRainEntranceSpeed*20);
        }
        */
    }

    void Sky::switchUnderwaterRain()
    {
        /*
        if (!mRainParticleSystem)
            return;

        bool freeze = mUnderwaterSwitch->isUnderwater();
        mRainParticleSystem->setFrozen(freeze);
        */
    }

    void Sky::setWeather(const WeatherResult& weather)
    {
        mRainEntranceSpeed = weather.mRainEntranceSpeed;
        mRainMaxRaindrops = weather.mRainMaxRaindrops;
        mRainDiameter = weather.mRainDiameter;
        mRainMinHeight = weather.mRainMinHeight;
        mRainMaxHeight = weather.mRainMaxHeight;
        mRainSpeed = weather.mRainSpeed;
        mWindSpeed = weather.mWindSpeed;
        mBaseWindSpeed = weather.mBaseWindSpeed;

        if (mRainEffect != weather.mRainEffect)
        {
            mRainEffect = weather.mRainEffect;
            if (!mRainEffect.empty())
            {
                // createRain();
            }
            else
            {
                // destroyRain();
            }
        }

        // updateRainParameters();

        mIsStorm = weather.mIsStorm;
        mStormDirection = weather.mStormDirection;

        mNextStormDirection = weather.mNextStormDirection;

        /*
        if (mDirtyParticlesEffect || (mCurrentParticleEffect != weather.mParticleEffect))
        {
            mDirtyParticlesEffect = false;
            mCurrentParticleEffect = weather.mParticleEffect;

            // cleanup old particles
            if (mParticleEffect)
            else
            {
                if (!mParticleNode)
                {
                    mParticleNode = new osg::PositionAttitudeTransform;
                    mParticleNode->addCullCallback(mUnderwaterSwitch);
                    mParticleNode->setNodeMask(Mask_WeatherParticles);
                    mRootNode->addChild(mParticleNode);
                }

                mParticleEffect = mSceneManager->getInstance(mCurrentParticleEffect, mParticleNode);

                SceneUtil::AssignControllerSourcesVisitor assignVisitor(std::make_shared<SceneUtil::FrameTimeSource>());
                mParticleEffect->accept(assignVisitor);

                SetupVisitor alphaFaderSetupVisitor(mPrecipitationAlpha);
                mParticleEffect->accept(alphaFaderSetupVisitor);

                SceneUtil::FindByClassVisitor findPSVisitor(std::string("ParticleSystem"));
                mParticleEffect->accept(findPSVisitor);

                for (unsigned int i = 0; i < findPSVisitor.mFoundNodes.size(); ++i)
                {
                    osgParticle::ParticleSystem *ps = static_cast<osgParticle::ParticleSystem
        *>(findPSVisitor.mFoundNodes[i]);

                    osg::ref_ptr<osgParticle::ModularProgram> program = new osgParticle::ModularProgram;
                    if (!mIsStorm)
                        program->addOperator(new WrapAroundOperator(mCamera,osg::Vec3(1024,1024,800)));
                    program->addOperator(new WeatherAlphaOperator(mPrecipitationAlpha, false));
                    program->setParticleSystem(ps);
                    mParticleNode->addChild(program);

                    for (int particleIndex = 0; particleIndex < ps->numParticles(); ++particleIndex)
                    {
                        ps->getParticle(particleIndex)->setAlphaRange(osgParticle::rangef(mPrecipitationAlpha,
        mPrecipitationAlpha)); ps->getParticle(particleIndex)->update(0, true);
                    }

                    ps->setUserValue("simpleLighting", true);
                }

                mSceneManager->recreateShaders(mParticleNode);
            }
        }
*/
        mClouds->setTexture(getOrCreateCloudTexture(weather.mCloudTexture), *mCompile);
        mClouds.setEnabled(mClouds->textureValid());
        mCloudBlendFactor = std::clamp(weather.mCloudBlendFactor, 0.f, 1.f);
        mClouds->material.setAlpha(1.f - mCloudBlendFactor);
        if (!weather.mNextCloudTexture.empty())
        {
            mNextClouds->setTexture(getOrCreateCloudTexture(weather.mNextCloudTexture), *mCompile);
            mNextClouds->material.setAlpha(mCloudBlendFactor);
        }
        mNextClouds.setEnabled(mCloudBlendFactor > 0.f && mNextClouds->textureValid());
        mCloudSpeed = weather.mCloudSpeed;

        mSkyColor = toVsg(weather.mSkyColor);
        mSkyColor.a = weather.mGlareView;

        mSun->setColor(toVsg(weather.mSunDiscColor));
        mSun->material.setAlpha(weather.mSunDiscColor.a());
        mSun->glareView = weather.mGlareView;

        if (weather.mNight)
            mAtmosphereNight->material.setAlpha(weather.mNightFade);
        mAtmosphereNight.setEnabled(weather.mNight);

        mPrecipitationAlpha = weather.mPrecipitationAlpha;
        mLightSize = 8 + (weather.mNightFade * 20);
    }

    void Sky::sunEnable()
    {
        mSun.setEnabled(true);
        mSun->enabled = true;
    }

    void Sky::sunDisable()
    {
        mSun.setEnabled(false);
        mSun->enabled = false;
    }

    void Sky::setStormParticleDirection(const osg::Vec3f& direction)
    {
        mStormParticleDirection = direction;
    }

    void Sky::setSunDirection(const osg::Vec3f& direction)
    {
        mSun->setDirection(toVsg(direction));
    }

    void Sky::setMasserState(const MoonState& state)
    {
        mMasser->setState(state);
        // mMasser.setEnabled(isBelowHorizon);
        // CullScreenSpaceQuad(
    }

    void Sky::setSecundaState(const MoonState& state)
    {
        mSecunda->setState(state);
        // mSecunda.setEnabled(isBelowHorizon);
    }

    void Sky::setDate(int day, int month)
    {
        mDay = day;
        mMonth = month;
    }

    void Sky::setGlareTimeOfDayFade(float val)
    {
        mSun->glareTimeOfDayFade = val;
    }

    vsg::ref_ptr<vsg::Descriptor> Sky::getOrCreateCloudTexture(const std::string& file)
    {
        auto found = mCloudTextures.find(file);
        if (found == mCloudTextures.end())
        {
            auto data = vsgUtil::readImage(file, mTextureOptions);
            auto sampler = vsg::Sampler::create();
            share(sampler, mTextureOptions);
            auto descriptor = vsg::DescriptorImage::create(sampler, data);
            found = mCloudTextures.emplace(file, descriptor).first;
        }
        return found->second;
    }
}
