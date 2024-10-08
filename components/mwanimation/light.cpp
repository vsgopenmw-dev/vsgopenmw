#include "light.hpp"

#include <optional>

#include <vsg/nodes/Light.h>

#include <components/animation/deltatime.hpp>
#include <components/animation/tmutable.hpp>
#include <components/esm3/loadligh.hpp>
#include <components/misc/rng.hpp>

#include "color.hpp"

namespace
{
    class LightController : public Anim::TMutable<LightController, vsg::Light>
    {
        Anim::DeltaTime mDt;
        float mBrightness = 0.675f;
        float mTicksToAdvance = 0.f;
        float mPhase;
        float mSpeed;
        int mType;
        float mBaseIntensity;

    public:
        LightController(int type, float baseIntensity)
            : mType(type)
            , mBaseIntensity(baseIntensity)
        {
            hints.autoPlay = true;
            mPhase = 0.25f + Misc::Rng::rollClosedProbability() * 0.75f;
            mSpeed = (type & ESM::Light::Flicker || type & ESM::Light::Pulse) ? 0.1f : 0.05f;
        }
        void apply(vsg::Light& light, float time)
        {
            // Updating flickering at 15 FPS like vanilla.
            constexpr float updateRate = 15.f;
            mTicksToAdvance = mDt.get(time) * updateRate * 0.25f + mTicksToAdvance * 0.75f;
            if (mBrightness >= mPhase)
                mBrightness -= mTicksToAdvance * mSpeed;
            else
                mBrightness += mTicksToAdvance * mSpeed;

            if (std::abs(mBrightness - mPhase) < mSpeed)
            {
                if (mType & ESM::Light::Flicker || mType & ESM::Light::FlickerSlow)
                    mPhase = 0.25f + Misc::Rng::rollClosedProbability() * 0.75f;
                else // if (mType == LT_Pulse || mType == LT_PulseSlow)
                    mPhase = mPhase <= 0.5f ? 1.f : 0.25f;
            }
            light.intensity = mBaseIntensity * mBrightness /* * actorFade*/;
        }
    };

    vsg::ref_ptr<vsg::PointLight> createLight(const ESM::Light& light)
    {
        auto node = vsg::PointLight::create();
        node->intensity = light.mData.mRadius;
        node->color = MWAnim::rgbColor(light.mData.mColor);
        if (light.mData.mFlags & ESM::Light::Negative)
            node->color *= -1;
        return node;
    }

    vsg::ref_ptr<LightController> createLightControllerIfRequired(const ESM::Light& light)
    {
        if (!(light.mData.mFlags
                & (ESM::Light::Flicker | ESM::Light::FlickerSlow | ESM::Light::Pulse | ESM::Light::PulseSlow)))
            return {};
        return LightController::create(light.mData.mFlags, light.mData.mRadius);
    }
}

namespace MWAnim
{
    vsg::Node* addLight(
        vsg::Group* attachLight, vsg::Group* fallback, const ESM::Light& light, Anim::Controllers& controllers)
    {
        if (!attachLight)
            attachLight = fallback;
        auto l = createLight(light);
        attachLight->addChild(l);
        if (auto ctrl = createLightControllerIfRequired(light))
        {
            ctrl->attachTo(*l);
            controllers.emplace_back(ctrl, l);
        }
        return l;
    }
}
