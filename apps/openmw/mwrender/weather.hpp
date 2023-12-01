#ifndef VSGOPENMW_MWRENDER_SKY_H
#define VSGOPENMW_MWRENDER_SKY_H

// vsgopenmw-move-me("sky.hpp")

#include <components/vsgutil/composite.hpp>
#include <components/vsgutil/toggle.hpp>

#include <memory>
#include <string>
#include <vector>

#include <osg/Vec4f>

namespace Resource
{
    class ResourceSystem;
}
namespace vsgUtil
{
    class CompileContext;
}
namespace MWRender
{
    class Atmosphere;
    class AtmosphereNight;
    class Clouds;
    class Sun;
    class Moon;
    class WeatherResult;
    class MoonState;

    /// Handles rendering of sky domes, celestial bodies as well as other objects that need to be rendered
    /// relative to the camera (e.g. weather particle effects)
    class Sky : public vsgUtil::Composite<vsg::Group>
    {
    public:
        Sky(Resource::ResourceSystem& resourceSystem, vsg::ref_ptr<vsgUtil::CompileContext> compile, bool occlusionQueryPrecise);
        ~Sky();

        vsg::ref_ptr<vsg::Node> resetNode();

        void update(float duration);

        void setEnabled(bool enabled);

        void setHour(double hour);
        ///< will be called even when sky is disabled.

        void setDate(int day, int month);
        ///< will be called even when sky is disabled.

        int getMasserPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        int getSecundaPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        void setMoonColour(bool red);
        ///< change Secunda colour to red

        void setWeather(const WeatherResult& weather);

        void setAtmosphereNightRoll(float roll);

        void sunEnable();

        void sunDisable();

        bool isEnabled();

        bool hasRain() const;

        float getRainIntensity() const;

        void setRainSpeed(float speed);

        void setStormParticleDirection(const osg::Vec3f& direction);

        void setSunDirection(const osg::Vec3f& direction);

        void setMasserState(const MoonState& state);
        void setSecundaState(const MoonState& state);

        void setGlareTimeOfDayFade(float val);

        float getBaseWindSpeed() const { return mBaseWindSpeed; }
        vsg::vec4 getSkyColor() const { return mSkyColor; }
        float getLightSize() const { return mLightSize; }

    private:
        vsg::ref_ptr<vsg::Descriptor> getOrCreateCloudTexture(const std::string& file);
        void createRain();
        void destroyRain();
        void switchUnderwaterRain();
        void updateRainParameters();

        vsg::ref_ptr<vsgUtil::CompileContext> mCompile;
        std::map<std::string, vsg::ref_ptr<vsg::Descriptor>> mCloudTextures;
        vsg::ref_ptr<const vsg::Options> mTextureOptions;
        vsg::ref_ptr<vsg::Switch> mSwitch;

        /*
        osg::ref_ptr<osg::PositionAttitudeTransform> mParticleNode;
        osg::ref_ptr<osg::Node> mParticleEffect;

        osg::ref_ptr<osgParticle::ParticleSystem> mRainParticleSystem;
        osg::ref_ptr<UnderwaterSwitchCallback> mUnderwaterSwitch;
        */

        std::unique_ptr<Atmosphere> mAtmosphere;
        vsgUtil::Toggle<AtmosphereNight> mAtmosphereNight;
        vsgUtil::Toggle<Clouds> mClouds;
        vsgUtil::Toggle<Clouds> mNextClouds;
        vsgUtil::Toggle<Sun> mSun;
        vsgUtil::Toggle<Moon> mMasser;
        vsgUtil::Toggle<Moon> mSecunda;

        bool mIsStorm = false;

        int mDay = 0;
        int mMonth = 0;

        float mCloudAnimationTimer = 0.f;

        float mRainTimer = 0.f;

        // particle system rotation is independent of cloud rotation internally
        osg::Vec3f mStormParticleDirection;
        osg::Vec3f mStormDirection;
        osg::Vec3f mNextStormDirection;

        float mCloudBlendFactor = 0.f;
        float mCloudSpeed = 0.f;
        vsg::vec4 mSkyColor;

        std::string mCurrentParticleEffect;

        bool mRainEnabled = false;
        std::string mRainEffect;
        float mRainSpeed = 0.f;
        float mRainDiameter = 0.f;
        float mRainMinHeight = 0.f;
        float mRainMaxHeight = 0.f;
        float mRainEntranceSpeed = 1.f;
        int mRainMaxRaindrops = 0;
        float mWindSpeed = 0.f;
        float mBaseWindSpeed = 0.f;

        float mGlareTimeOfDayFade = 0.f;

        float mPrecipitationAlpha = 0.f;
        bool mDirtyParticlesEffect = false;

        vsg::vec4 mMoonScriptColor;
        float mLightSize{};
    };
}

#endif
