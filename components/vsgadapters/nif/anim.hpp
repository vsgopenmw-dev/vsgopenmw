#ifndef VSGOPENMW_VSGADAPTERS_NIF_ANIM_H
#define VSGOPENMW_VSGADAPTERS_NIF_ANIM_H

#include <optional>

#include <components/animation/constant.hpp>
#include <components/animation/extrapolate.hpp>
#include <components/animation/interpolate.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/data.hpp>
#include <components/nif/extra.hpp>
#include <components/nif/nifkey.hpp>
#include <components/nif/node.hpp>
#include <components/vsgadapters/osgcompat.hpp>

namespace Anim
{
    class Tags;
    class Transform;
    class TransformController;
    class Roll;
}
namespace vsgAdapters
{
    template <class Channel, class T>
    inline vsg::ref_ptr<Channel> handleKeyData(const T& src)
    {
        auto ret = Anim::make_channel<Channel>();
        for (auto& [key, val] : src.mKeys)
            ret->keys[key] = toVsg(val.mValue);
        return ret;
    }

    template <class Channel, class T>
    inline vsg::ref_ptr<Channel> handleQuadraticKeyData(const T& src)
    {
        auto ret = Anim::make_channel<Channel>();
        for (auto& [key, val] : src.mKeys)
            ret->keys[key] = { toVsg(val.mValue), toVsg(val.mInTan), toVsg(val.mOutTan) };
        return ret;
    }

    template <>
    inline vsg::ref_ptr<Anim::CubicHermiteSpline<vsg::quat>> handleQuadraticKeyData(const Nif::QuaternionKeyMap& src)
    {
        return handleKeyData<Anim::CubicHermiteSpline<vsg::quat>>(src);
    }

    inline Anim::ExtrapolationMode convertExtrapolationMode(int mode)
    {
        switch (mode)
        {
            case Nif::NiTimeController::ExtrapolationMode::Cycle:
                return Anim::ExtrapolationMode::Cycle;
            case Nif::NiTimeController::ExtrapolationMode::Reverse:
                return Anim::ExtrapolationMode::Reverse;
            case Nif::NiTimeController::ExtrapolationMode::Constant:
            default:
                return Anim::ExtrapolationMode::Constant;
        }
    }

    template <class T>
    inline void addExtrapolatorIfRequired(
        const Nif::NiTimeController& ctrl, Anim::channel_ptr<T>& channel, float startKey, float stopKey)
    {
        Anim::ExtrapolationMode mode = convertExtrapolationMode(ctrl.extrapolationMode());

        if (ctrl.mTimeStop - ctrl.mTimeStart <= 0)
        {
            channel = Anim::make_constant(channel->value(ctrl.mTimeStart)); // vsgopenmw-optimization-nif-constant-channel
            return;
        }
        if (ctrl.mFrequency == 1 && ctrl.mPhase == 0 && mode == Anim::ExtrapolationMode::Constant && ctrl.mTimeStart <= startKey && ctrl.mTimeStop >= stopKey)
            return; // vsgopenmw-optimization-nif-prune-extrapolator

        channel = Anim::makeExtrapolator<T>(channel, { ctrl.mFrequency, ctrl.mPhase, ctrl.mTimeStart, ctrl.mTimeStop }, mode);
    }

    template <class T, class Src>
    inline Anim::channel_ptr<T> handleInterpolatedKeyframes(const Src& src)
    {
        switch (src->mInterpolationType)
        {
            case Nif::InterpolationType_Constant:
                return handleKeyData<Anim::Flip<T>>(*src);
            case Nif::InterpolationType_Linear:
            default:
                return handleKeyData<Anim::Linear<T>>(*src);
            case Nif::InterpolationType_Quadratic:
                return handleQuadraticKeyData<Anim::CubicHermiteSpline<T>>(*src);
        }
    }

    template <class T, class Src>
    inline Anim::channel_ptr<T> handleKeyframes(
        const Nif::NiTimeController& ctrl, const Src& src, const std::optional<T> defaultValue = {})
    {
        if (!src || src->mKeys.empty())
        {
            if (defaultValue)
                return Anim::make_constant(*defaultValue);
            return {};
        }
        if (src->mKeys.size() == 1)
            return Anim::make_constant(toVsg(src->mKeys.begin()->second.mValue)); // vsgopenmw-optimization-nif-constant-channel

        auto ret = handleInterpolatedKeyframes<T>(src);
        float startKey = src->mKeys.begin()->first;
        float stopKey = src->mKeys.rbegin()->first;
        addExtrapolatorIfRequired(ctrl, ret, startKey, stopKey);
        return ret;
    }

    template <class F>
    void callActiveControllers(const Nif::NiTimeControllerPtr controller, F func)
    {
        for (Nif::NiTimeControllerPtr ctrl = controller; !ctrl.empty() && ctrl->isActive(); ctrl = ctrl->mNext)
            func(*(ctrl.getPtr()));
    }

    template <class T>
    const T* searchController(const Nif::NiTimeControllerPtr controller, int recType)
    {
        for (Nif::NiTimeControllerPtr ctrl = controller; !ctrl.empty() && ctrl->isActive(); ctrl = ctrl->mNext)
            if (ctrl->recType == recType)
                return static_cast<const T*>(ctrl.getPtr());
        return nullptr;
    }

    void convertTrafo(Anim::Transform& trans, const Nif::NiTransform& trafo);

    vsg::ref_ptr<Anim::Tags> handleTextKeys(const Nif::NiTextKeyExtraData& keys);

    vsg::ref_ptr<Anim::TransformController> handleKeyframeController(const Nif::NiKeyframeController& keyctrl);
    vsg::ref_ptr<Anim::TransformController> handlePathController(const Nif::NiPathController& pathctrl);
    vsg::ref_ptr<Anim::Roll> handleRollController(const Nif::NiRollController& rollctrl);
    bool hasTransformController(const Nif::NiAVObject& n);
}

#endif
