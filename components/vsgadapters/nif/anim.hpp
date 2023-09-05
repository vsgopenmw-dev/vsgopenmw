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
            case Nif::Controller::ExtrapolationMode::Cycle:
                return Anim::ExtrapolationMode::Cycle;
            case Nif::Controller::ExtrapolationMode::Reverse:
                return Anim::ExtrapolationMode::Reverse;
            case Nif::Controller::ExtrapolationMode::Constant:
            default:
                return Anim::ExtrapolationMode::Constant;
        }
    }

    template <class T>
    inline void addExtrapolatorIfRequired(
        const Nif::Controller& ctrl, Anim::channel_ptr<T>& channel, float startKey, float stopKey)
    {
        Anim::ExtrapolationMode mode = convertExtrapolationMode((ctrl.flags & 0x6) >> 1);

        if (ctrl.timeStop - ctrl.timeStart <= 0)
        {
            channel = Anim::make_constant(channel->value(ctrl.timeStart)); // vsgopenmw-optimization-nif-constant-channel
            return;
        }
        if (ctrl.frequency == 1 && ctrl.phase == 0 && mode == Anim::ExtrapolationMode::Constant && ctrl.timeStart <= startKey && ctrl.timeStop >= stopKey)
            return; // vsgopenmw-optimization-nif-prune-extrapolator

        channel = Anim::makeExtrapolator<T>(channel, { ctrl.frequency, ctrl.phase, ctrl.timeStart, ctrl.timeStop }, mode);
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
        const Nif::Controller& ctrl, const Src& src, const std::optional<T> defaultValue = {})
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
    void callActiveControllers(const Nif::ControllerPtr controller, F func)
    {
        for (Nif::ControllerPtr ctrl = controller; !ctrl.empty() && ctrl->isActive(); ctrl = ctrl->next)
            func(*(ctrl.getPtr()));
    }

    template <class T>
    const T* searchController(const Nif::ControllerPtr controller, int recType)
    {
        for (Nif::ControllerPtr ctrl = controller; !ctrl.empty() && ctrl->isActive(); ctrl = ctrl->next)
            if (ctrl->recType == recType)
                return static_cast<const T*>(ctrl.getPtr());
        return nullptr;
    }

    void convertTrafo(Anim::Transform& trans, const Nif::Transformation& trafo);

    vsg::ref_ptr<Anim::Tags> handleTextKeys(const Nif::NiTextKeyExtraData& keys);

    vsg::ref_ptr<Anim::TransformController> handleKeyframeController(const Nif::NiKeyframeController& keyctrl);
    vsg::ref_ptr<Anim::TransformController> handlePathController(const Nif::NiPathController& pathctrl);
    vsg::ref_ptr<Anim::Roll> handleRollController(const Nif::NiRollController& rollctrl);
    bool hasTransformController(const Nif::Node& n);
}

#endif
