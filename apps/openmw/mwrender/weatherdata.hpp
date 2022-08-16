#ifndef VSGOPENMW_MWRENDER_WEATHERDATA_H
#define VSGOPENMW_MWRENDER_WEATHERDATA_H

#include <osg/Vec4f>
#include <string>

namespace MWRender
{
    struct WeatherResult
    {
        size_t mCloudTexture;
        size_t mNextCloudTexture;
        float mCloudBlendFactor;

        osg::Vec4f mFogColor;

        osg::Vec4f mAmbientColor;

        osg::Vec4f mSkyColor;

        // sun light color
        osg::Vec4f mSunColor;

        // alpha is the sun transparency
        osg::Vec4f mSunDiscColor;

        float mFogDepth;

        float mDLFogFactor;
        float mDLFogOffset;

        float mWindSpeed;
        float mBaseWindSpeed;
        float mCurrentWindSpeed;
        float mNextWindSpeed;

        float mCloudSpeed;

        float mGlareView;

        bool mNight; // use night skybox
        float mNightFade; // fading factor for night skybox

        bool mIsStorm;

        std::string mAmbientLoopSoundID;
        float mAmbientSoundVolume;

        std::string mParticleEffect;
        std::string mRainEffect;
        float mPrecipitationAlpha;

        float mRainDiameter;
        float mRainMinHeight;
        float mRainMaxHeight;
        float mRainSpeed;
        float mRainEntranceSpeed;
        int mRainMaxRaindrops;

        osg::Vec3f mStormDirection;
        osg::Vec3f mNextStormDirection;
    };

    struct MoonState
    {
        enum class Phase
        {
            Full = 0,
            WaningGibbous,
            ThirdQuarter,
            WaningCrescent,
            New,
            WaxingCrescent,
            FirstQuarter,
            WaxingGibbous,
            Count
        };

        float mRotationFromHorizon;
        float mRotationFromNorth;
        Phase mPhase;
        float mShadowBlend;
        float mMoonAlpha;
    };
}

#endif
