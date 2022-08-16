#include "weather.hpp"

#include <vsg/io/Options.h>
#include <vsg/utils/SharedObjects.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/DepthSorted.h>
#include <vsg/state/StateSwitch.h>
#include <vsg/state/BindDynamicDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/commands/Draw.h>
#include <vsg/commands/BeginQuery.h>
#include <vsg/commands/EndQuery.h>
#include <vsg/commands/ResetQueryPool.h>

#include <components/resource/resourcesystem.hpp>
#include <components/settings/settings.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/vsgutil/searchbytype.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/readnode.hpp>
#include <components/vsgutil/readshader.hpp>
#include <components/vsgutil/viewrelative.hpp>
#include <components/vsgutil/setbin.hpp>
#include <components/pipeline/builder.hpp>
#include <components/pipeline/graphics.hpp>
#include <components/pipeline/material.hpp>
#include <components/pipeline/override.hpp>
#include <components/pipeline/sets.hpp>
#include <components/animation/transform.hpp>
#include <components/animation/clone.hpp>
#include <components/fallback/fallback.hpp>

#include "../mwworld/weather.hpp"

#include "bin.hpp"
#include "weatherdata.hpp"

namespace
{
    void depthOff(Pipeline::Options &o)
    {
        o.depthTest = false; o.depthWrite = false;
    }

    static const auto sDrawQuad = vsg::Draw::create(4,1,0,0);
}

namespace MWRender
{
    class Atmosphere
    {
    public:
        Pipeline::Material material;
        static void pipelineOptions(Pipeline::Options &o)
        {
            o.shader = "sky/atmosphere/";
            depthOff(o);
        }
    };

    class AtmosphereNight : public Anim::Transform //vsgUtil::Composite<Anim::Transform> // ToggleComposite
    {
    public:
        Pipeline::Material material;
        static void pipelineOptions(Pipeline::Options &o)
        {
            o.shader = "sky/night/";
            depthOff(o);
        }
    };

    class Clouds : public Anim::Transform
    {
        vsg::ref_ptr<vsg::StateSwitch> mStateSwitch;
        vsg::ref_ptr<const vsg::BindDescriptorSet> mReplaceBds;
    public:
        Pipeline::Material material;
        Pipeline::DynamicDescriptorValue<vsg::mat4> texmat{Pipeline::Descriptors::TEXMAT_BINDING};
        Clouds(vsg::ref_ptr<vsg::Node> node)
        {
            mStateSwitch = vsg::StateSwitch::create();

            auto replaceBds = [this] (const vsg::Node &n) -> vsg::ref_ptr<vsg::Node> {
                if (auto *bds = dynamic_cast<const vsg::BindDescriptorSet*>(&n))
                {
                    if (bds->pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS)
                        return {};
                    mReplaceBds = bds;
                    return mStateSwitch;
                }
                return {};
            };
            Anim::cloneIfRequired(node, replaceBds);
            addChild(node);
        }
        static void pipelineOptions(Pipeline::Options &o)
        {
            depthOff(o);
            o.blend = true;
            o.shader = "sky/clouds/";
            o.modes = {};
            o.addMode(Pipeline::Mode::DIFFUSE_MAP);
            o.addMode(Pipeline::Mode::TEXMAT);
            o.addMode(Pipeline::Mode::MATERIAL);
        }
        void setTexture(size_t tex)
        {
            for (size_t i = 0; i<mStateSwitch->children.size(); ++i)
                mStateSwitch->children[i].mask = vsg::boolToMask(i==tex);
        }
        size_t numTextures()
        {
            return mStateSwitch->children.size();
        }
        void setTextures(const vsg::Descriptors &textures)
        {
            if (!mReplaceBds) return;
            for (auto tex : textures)
            {
                auto set = vsg::DescriptorSet::create(mReplaceBds->descriptorSet->setLayout, vsg::Descriptors{tex, material.descriptor(), texmat.descriptor()});
                auto bds = vsg::BindDynamicDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, mReplaceBds->layout, mReplaceBds->firstSet, set);
                mStateSwitch->add(vsg::boolToMask(mStateSwitch->children.empty()), bds);
                mStateSwitch->slot = bds->slot;
            }
        }
        void update()
        {
            material.copyDataListToBuffers();
            texmat.copyDataListToBuffers();
        }
    };

    class CelestialBody : public Anim::Transform
    {
    public:
        Pipeline::Material material;
        CelestialBody(float scaleFactor=1.f)
        {
            scale = vsg::vec3(450,450,450) * scaleFactor;
        }
        Pipeline::Options pipelineOptions(const std::string &shader)
        {
            auto options = Pipeline::Options{.shader=shader, .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, .blend = true, .cullMode = VK_CULL_MODE_NONE};
            depthOff(options);
            options.addMode(Pipeline::Mode::MATERIAL);
            options.addMode(Pipeline::Mode::DIFFUSE_MAP);
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
    public:
        Pipeline::Material material;
        float mFaderMax;
        SunGlare(vsg::ref_ptr<const vsg::Options> shaderOptions)
        {
            mNode = vsg::StateGroup::create();
            auto color = toVsg(Fallback::Map::getColour("Weather_Sun_Glare_Fader_Color"));
            // Replicating a design flaw in MW. The color was being set on both ambient and emissive properties, which multiplies the result by two,
            // then finally gets clamped by the fixed function pipeline. With the default INI settings, only the red component gets clamped,
            // so the resulting color looks more orange than red.
            color *= 2;
            for (int i=0; i<2; ++i)
                color[i] = std::min(1.f, color[i]);
            mFaderMax = Fallback::Map::getFloat("Weather_Sun_Glare_Fader_Max");
            auto faderAngleMax = Fallback::Map::getFloat("Weather_Sun_Glare_Fader_Angle_Max");
            /*material.faderAngleMax*/color.a = faderAngleMax;
            material.value().emissive = color;

            Pipeline::Builder builder(shaderOptions);
            auto pipelineOptions = Pipeline::Options{.shader="sky/sunglare/", .blend = true, .cullMode = VK_CULL_MODE_NONE};
            depthOff(pipelineOptions);
            pipelineOptions.addMode(Pipeline::Mode::MATERIAL);

            auto pipeline = builder.graphics->getOrCreate(pipelineOptions);
            auto bds = vsg::BindDynamicDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline->layout, Pipeline::TEXTURE_SET, vsg::DescriptorSet::create(pipeline->pipeline->layout->setLayouts[Pipeline::TEXTURE_SET], vsg::Descriptors{material.descriptor()}));
            mNode->stateCommands = {pipeline, bds};
            mNode->children = {vsgUtil::SetBin::create(Bin_SunGlare, sDrawQuad)};
        }
        void update(float alpha)
        {
            material.setAlpha(mFaderMax * alpha);
            material.copyDataListToBuffers();
        }
    };

    class Sun : public CelestialBody
    {
        Pipeline::Material sunFlashMaterial;
        vsg::ref_ptr<vsg::QueryPool> mQueryPool;
        const uint32_t mQueryCount = 2;
        vsg::ref_ptr<Anim::Transform> mFlash;
        vsg::ref_ptr<vsg::Switch> mSwitch;
        std::unique_ptr<SunGlare> mGlare;
    public:
        float glareView = 1.f;
        float glareTimeOfDayFade = 1.f;
        float visibleRatio = 0.f;
        Sun(vsg::ref_ptr<const vsg::Options> textureOptions, vsg::ref_ptr<const vsg::Options> shaderOptions)
        {
            {
                mQueryPool = vsg::QueryPool::create();
                mQueryPool->queryType = VK_QUERY_TYPE_OCCLUSION;
                mQueryPool->queryCount = mQueryCount;
                addChild(vsgUtil::SetBin::create(Bin_Compute, vsg::ResetQueryPool::create(mQueryPool)));
                addChild(vsg::BeginQuery::create(mQueryPool, 0, 0));
            }

            auto builder = Pipeline::Builder(shaderOptions);
            auto sunTex = vsgUtil::readDescriptorImage("tx_sun_05.dds", textureOptions, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
            auto pipeline = builder.graphics->getOrCreate(pipelineOptions("sky/sun/"));
            {
                auto sg = vsg::StateGroup::create();
                auto bds = vsg::BindDynamicDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline->layout, Pipeline::TEXTURE_SET, vsg::DescriptorSet::create(pipeline->pipeline->layout->setLayouts[Pipeline::TEXTURE_SET], vsg::Descriptors{sunTex, material.descriptor()}));
                sg->stateCommands = {pipeline, bds};
                sg->children = {sDrawQuad};
                addChild(sg);
            }
            addChild(vsg::EndQuery::create(mQueryPool, 0));

            auto binGroup = vsg::Group::create();
            binGroup->addChild(vsg::BeginQuery::create(mQueryPool, 1, 0));
            {
                auto queryPipeline = builder.graphics->getOrCreate(queryOptions());
                auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, queryPipeline->pipeline->layout, Pipeline::TEXTURE_SET, vsg::DescriptorSet::create(queryPipeline->pipeline->layout->setLayouts[Pipeline::TEXTURE_SET], vsg::Descriptors{sunTex}));
                auto sg = vsg::StateGroup::create();
                sg->stateCommands = {queryPipeline, bds};
                sg->children = {sDrawQuad};
                binGroup->addChild(sg);
            }
            binGroup->addChild(vsg::EndQuery::create(mQueryPool, 1));

            mSwitch = vsg::Switch::create();
            {
                auto sg = vsg::StateGroup::create();
                auto flashTex = vsgUtil::readDescriptorImage("tx_sun_flash_grey_05.dds", textureOptions, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
                sunFlashMaterial.value().emissive = {1,1,1,1};
                auto bds = vsg::BindDynamicDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline->layout, Pipeline::TEXTURE_SET, vsg::DescriptorSet::create(pipeline->pipeline->layout->setLayouts[Pipeline::TEXTURE_SET], vsg::Descriptors{flashTex, sunFlashMaterial.descriptor()}));
                sg->stateCommands = {pipeline, bds};
                mFlash = vsg::ref_ptr{new Anim::Transform};
                mFlash->children = {sDrawQuad};
                sg->children = {mFlash};
                mSwitch->addChild(false, sg);
            }
            mGlare = std::make_unique<SunGlare>(shaderOptions);
            mSwitch->addChild(false, mGlare->node());
            binGroup->addChild(mSwitch);

            addChild(vsgUtil::SetBin::create(Bin_SunGlare, binGroup));
        }
        Pipeline::Options queryOptions()
        {
            auto options = Pipeline::Options{.shader="sky/query/", .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, .colorWrite=false, .cullMode = VK_CULL_MODE_NONE};
            options.addMode(Pipeline::Mode::DIFFUSE_MAP);
            return options;
        }
        void setColor(const vsg::vec4 &color)
        {
            material.value().emissive = color;
        }
        void updateQuery()
        {
            std::vector<uint64_t> results(mQueryCount);
            if (mQueryPool->getResults(results, 0) != VK_SUCCESS)
                return;
            auto total = results[0];
            auto visible = results[1];
            visibleRatio = total > 0 ? static_cast<float>(visible)/total : 0.f;
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
                    sunFlashMaterial.value().diffuse.a = fade*glareView;
                }
                else if (visibleRatio < 1.f)
                {
                    sunFlashMaterial.value().diffuse.a = glareView;
                    const float threshold = 0.6;
                    scale *= visibleRatio * (1.f - threshold) + threshold;
                }
                mFlash->setScale(scale);
            }
            sunFlashMaterial.copyDataListToBuffers();
        }
        void update()
        {
            updateQuery();

            auto glare = glareView * visibleRatio;
            mSwitch->setAllChildren(glare > 0.f);

            mGlare->update(glare * glareTimeOfDayFade);
            updateFlash();

            material.copyDataListToBuffers();
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

        Moon(vsg::ref_ptr<const vsg::Options> options, float scaleFactor, Type type, const Pipeline::Builder &builder)
            : CelestialBody(scaleFactor)
            , mType(type)
        {
            mCircleTex = vsgUtil::readDescriptorImage(std::string("tx_mooncircle_full_") + (type == Type::Masser ? "m" : "s") + ".dds", options, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, Pipeline::Descriptors::DIFFUSE_UNIT);

            auto po = pipelineOptions("sky/moon/");
            po.addMode(Pipeline::Mode::GLOW_MAP);
            auto sg = vsg::StateGroup::create();
            mPipeline = builder.graphics->getOrCreate(po);
            mStateSwitch = vsg::StateSwitch::create();
            for (uint32_t i=0; i<static_cast<uint32_t>(MoonState::Phase::Count); ++i)
            {
                auto bds = createBindDescriptorSet(options, static_cast<MoonState::Phase>(i), builder);
                mStateSwitch->add(vsg::boolToMask(i==0), bds);
                mStateSwitch->slot = bds->slot;
            }
            sg->stateCommands = {mPipeline, mStateSwitch};
            sg->children = {sDrawQuad};
            children = {sg};
        }
        std::string phaseName (MoonState::Phase phase)
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
        vsg::ref_ptr<vsg::BindDescriptorSet> createBindDescriptorSet(vsg::ref_ptr<const vsg::Options> options, MoonState::Phase phase, const Pipeline::Builder &builder)
        {
            std::string textureName = "tx_";
            if (mType == Moon::Type::Secunda)
                textureName += "secunda_";
            else
                textureName += "masser_";
            textureName += phaseName(phase) + ".dds";
            auto descriptors = vsg::Descriptors{
                vsgUtil::readDescriptorImage(textureName, options, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, Pipeline::Descriptors::GLOW_UNIT),
                mCircleTex,
                material.descriptor()
            };
            return vsg::BindDynamicDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->pipeline->layout, Pipeline::TEXTURE_SET, vsg::DescriptorSet::create(mPipeline->pipeline->layout->setLayouts[Pipeline::TEXTURE_SET], descriptors));
        }
        void setColor(const vsg::vec4 &color)
        {
            material.value().emissive = vsg::vec4(color.x, color.y, color.z, material.value().emissive.a);
        }
        void setAtmosphereColor(const vsg::vec4 &color)
        {
            material.value().ambient = color;
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
            material.value().emissive.a = state.mShadowBlend;
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
            for (uint32_t i=0; i<static_cast<uint32_t>(MoonState::Phase::Count); ++i)
                mStateSwitch->children[i].mask = vsg::boolToMask(static_cast<MoonState::Phase>(i) == phase);
        }
    private:
        vsg::ref_ptr<vsg::DescriptorImage> mCircleTex;
        vsg::ref_ptr<vsg::StateSwitch> mStateSwitch;
        vsg::ref_ptr<vsg::BindGraphicsPipeline> mPipeline;
        Type mType;
        MoonState::Phase mPhase;
    };

    Sky::Sky(Resource::ResourceSystem &resourceSystem)
        : mTextureOptions(resourceSystem.textureOptions)
        , mStormParticleDirection(MWWorld::Weather::defaultDirection())
        , mStormDirection(MWWorld::Weather::defaultDirection())
    {
        mNode = new vsgUtil::ViewRelative;
        auto nodeOptions = vsg::Options::create(*resourceSystem.nodeOptions);
        auto override_ = vsg::ref_ptr{new Pipeline::Override};
        override_->attachTo(*nodeOptions);
        mAtmosphere = std::make_unique<Atmosphere>();
        override_->material = mAtmosphere->material.descriptor();
        {
            override_->pipelineOptions = Atmosphere::pipelineOptions;
            mNode->addChild(vsgUtil::readNode(Settings::Manager::getString("skyatmosphere", "Models"), nodeOptions));
        }
        override_->material = {};
        mSwitch = vsg::Switch::create();
        mNode->addChild(mSwitch);
        {
            override_->pipelineOptions = AtmosphereNight::pipelineOptions;
            auto t = vsg::ref_ptr{new AtmosphereNight};
            override_->material = t->material.descriptor();
            auto node = vsgUtil::readNode(Settings::Manager::getString("skynight02", "Models"), nodeOptions, false);
            if (!node)
                node = vsgUtil::readNode(Settings::Manager::getString("skynight01", "Models"), nodeOptions);
            t->children = {node};
            mAtmosphereNight.setup(t, mSwitch, false);
            override_->material = {};
        }
        mSun.setup(vsg::ref_ptr{new Sun(resourceSystem.textureOptions, resourceSystem.shaderOptions)}, mSwitch, true);

        mMasser.setup(vsg::ref_ptr{new Moon(resourceSystem.textureOptions, Fallback::Map::getFloat("Moons_Masser_Size")/125, Moon::Type::Masser, *resourceSystem.builder)}, mSwitch, true);
        mSecunda.setup(vsg::ref_ptr{new Moon(resourceSystem.textureOptions, Fallback::Map::getFloat("Moons_Secunda_Size")/125, Moon::Type::Secunda, *resourceSystem.builder)}, mSwitch, true);
        mMoonScriptColor = toVsg(Fallback::Map::getColour("Moons_Script_Color"));

        {
            override_->pipelineOptions = Clouds::pipelineOptions;
            auto node = vsgUtil::readNode(Settings::Manager::getString("skyclouds", "Models"), nodeOptions);
            mClouds.setup(vsg::ref_ptr{new Clouds(node)}, mSwitch, true);
            mNextClouds.setup(vsg::ref_ptr{new Clouds(node)}, mSwitch, false);
            //shareDescriptorSet();
        }
        //mUnderwaterSwitch = new UnderwaterSwitchCallback(skyroot);
    }

    Sky::~Sky()
    {
    }

    void Sky::setResources(const std::vector<std::string> &cloudTextures, const std::vector<std::string> &particleEffects)
    {
        if (mClouds->numTextures())
            return;

        vsg::Descriptors textures;
        for (auto &filename : cloudTextures)
            textures.emplace_back(vsgUtil::readDescriptorImage(filename, mTextureOptions));
        mClouds->setTextures(textures);
        mNextClouds->setTextures(textures);

        for (auto &effect : particleEffects)
        {
        }
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

        osg::ref_ptr<osg::Texture2D> raindropTex = new osg::Texture2D(mSceneManager->getImageManager()->getImage("textures/tx_raindrop_01.dds"));
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

        // FIXME: vanilla engine does not use a particle system to handle rain, it uses a NIF-file with 20 raindrops in it.
        // It spawns the (maxRaindrops-getParticleSystem()->numParticles())*dt/rainEntranceSpeed batches every frame (near 1-2).
        // Since the rain is a regular geometry, it produces water ripples, also in theory it can be removed if collides with something.
        osg::ref_ptr<RainCounter> counter = new RainCounter;
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

    bool Sky::isEnabled()
    {
        return mEnabled;
    }

    bool Sky::hasRain() const
    {
        return false;// mRainNode != nullptr;
    }

    float Sky::getPrecipitationAlpha() const
    {
        if (mEnabled && !mIsStorm && (hasRain() /*|| mParticleNode*/))
            return mPrecipitationAlpha;
        return 0.f;
    }

    void Sky::update(float duration)
    {
        if (!mEnabled)
            return;

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
        vsg::mat4 texmat = vsg::translate(vsg::vec3(0,-mCloudAnimationTimer,0));
        mClouds->texmat.value() = texmat;
        mNextClouds->texmat.value() = texmat;

        // morrowind rotates each cloud mesh independently
        osg::Quat rotation;
        rotation.makeRotate(MWWorld::Weather::defaultDirection(), mStormDirection);
        mClouds->setAttitude(toVsg(rotation));

        rotation.makeRotate(MWWorld::Weather::defaultDirection(), mNextStormDirection);
        mNextClouds->setAttitude(toVsg(rotation));

        mSun->update();
        mClouds->update();
        mNextClouds->update();

        mAtmosphere->material.copyDataListToBuffers();
        mAtmosphereNight->material.copyDataListToBuffers();
        mMasser->material.copyDataListToBuffers();
        mSecunda->material.copyDataListToBuffers();
    }

    void Sky::setAtmosphereNightRoll(float roll)
    {
        mAtmosphereNight->setAttitude(vsg::quat(roll, vsg::vec3(0,0,1)));
    }

    void Sky::setEnabled(bool enabled)
    {
        /*
        if (!enabled && mParticleNode && mParticleEffect)
        {
            mCurrentParticleEffect.clear();
            mDirtyParticlesEffect = true;
        }
*/
        mEnabled = enabled;
    }

    void Sky::setMoonColour (bool red)
    {
        mSecunda->setColor(red ? mMoonScriptColor : vsg::vec4(1,1,1,1));
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
        /*
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
                createRain();
            }
            else
            {
                destroyRain();
            }
        }

        updateRainParameters();
        */

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
                    osgParticle::ParticleSystem *ps = static_cast<osgParticle::ParticleSystem *>(findPSVisitor.mFoundNodes[i]);

                    osg::ref_ptr<osgParticle::ModularProgram> program = new osgParticle::ModularProgram;
                    if (!mIsStorm)
                        program->addOperator(new WrapAroundOperator(mCamera,osg::Vec3(1024,1024,800)));
                    program->addOperator(new WeatherAlphaOperator(mPrecipitationAlpha, false));
                    program->setParticleSystem(ps);
                    mParticleNode->addChild(program);

                    for (int particleIndex = 0; particleIndex < ps->numParticles(); ++particleIndex)
                    {
                        ps->getParticle(particleIndex)->setAlphaRange(osgParticle::rangef(mPrecipitationAlpha, mPrecipitationAlpha));
                        ps->getParticle(particleIndex)->update(0, true);
                    }

                    ps->setUserValue("simpleLighting", true);
                }

                mSceneManager->recreateShaders(mParticleNode);
            }
        }
*/
        mClouds->setTexture(weather.mCloudTexture);
        mNextClouds->setTexture(weather.mNextCloudTexture);
        mCloudBlendFactor = std::clamp(weather.mCloudBlendFactor, 0.f, 1.f);

        mClouds->material.setAlpha(1.f - mCloudBlendFactor);
        mNextClouds->material.setAlpha(mCloudBlendFactor);
        mNextClouds.setEnabled(mCloudBlendFactor > 0.f);
        mCloudSpeed = weather.mCloudSpeed;

        auto skyColor = toVsg(weather.mSkyColor);
        skyColor.a = weather.mGlareView;
        mMasser->setAtmosphereColor(skyColor);
        mSecunda->setAtmosphereColor(skyColor);
        mAtmosphere->material.value().emissive = skyColor;
        mAtmosphere->material.value().ambient = toVsg(weather.mFogColor);

        mSun->material.value().emissive = toVsg(weather.mSunDiscColor);
        mSun->material.setAlpha(weather.mGlareView * weather.mSunDiscColor.a());
        mSun->glareView = weather.mGlareView;
        mAtmosphereNight->material.setAlpha(weather.mNightFade * weather.mGlareView);
        mAtmosphereNight.setEnabled(weather.mNight);
        mPrecipitationAlpha = weather.mPrecipitationAlpha;
    }

    void Sky::sunEnable()
    {
        mSun.setEnabled(true);
    }

    void Sky::sunDisable()
    {
        mSun.setEnabled(false);
    }

    void Sky::setStormParticleDirection(const osg::Vec3f &direction)
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
        //mMasser.setEnabled(isBelowHorizon);
    }

    void Sky::setSecundaState(const MoonState& state)
    {
        mSecunda->setState(state);
        //mSecunda.setEnabled(isBelowHorizon);
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

    void Sky::setWaterHeight(float height)
    {
        //mUnderwaterSwitch->setWaterLevel(height);
    }

    void Sky::setWaterEnabled(bool enabled)
    {
        //mUnderwaterSwitch->setEnabled(enabled);
    }
}
