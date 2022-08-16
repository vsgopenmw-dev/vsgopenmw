#ifndef VSGOPENMW_MWRENDER_SKY_H
#define VSGOPENMW_MWRENDER_SKY_H

//vsgopenmw-move-me("sky.hpp")

#include <components/vsgutil/toggle.hpp>
#include <components/vsgutil/composite.hpp>

#include <string>
#include <memory>
#include <vector>

#include <osg/Vec4f>

namespace Resource
{
    class ResourceSystem;
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
        Sky(Resource::ResourceSystem &resourceSystem);
        ~Sky();

        void setResources(const std::vector<std::string> &cloudTextures, const std::vector<std::string> &particleEffects);

        void update(float duration);

        void setEnabled(bool enabled);

        void setHour (double hour);
        ///< will be called even when sky is disabled.

        void setDate (int day, int month);
        ///< will be called even when sky is disabled.

        int getMasserPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        int getSecundaPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        void setMoonColour (bool red);
        ///< change Secunda colour to red

        void setWeather(const WeatherResult& weather);

        void setAtmosphereNightRoll(float roll);

        void sunEnable();

        void sunDisable();

        bool isEnabled();

        bool hasRain() const;

        float getPrecipitationAlpha() const;

        void setRainSpeed(float speed);

        void setStormParticleDirection(const osg::Vec3f& direction);

        void setSunDirection(const osg::Vec3f& direction);

        void setMasserState(const MoonState& state);
        void setSecundaState(const MoonState& state);

        void setGlareTimeOfDayFade(float val);

        /// Enable or disable the water plane (used to remove underwater weather particles)
        void setWaterEnabled(bool enabled);

        /// Set height of water plane (used to remove underwater weather particles)
        void setWaterHeight(float height);

        float getBaseWindSpeed() const { return mBaseWindSpeed; }

    private:
        void createRain();
        void destroyRain();
        void switchUnderwaterRain();
        void updateRainParameters();

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
        osg::Vec4f mSkyColour;

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

        bool mEnabled = true;

        float mPrecipitationAlpha = 0.f;
        bool mDirtyParticlesEffect = false;

        vsg::vec4 mMoonScriptColor;
    };
}

#endif
