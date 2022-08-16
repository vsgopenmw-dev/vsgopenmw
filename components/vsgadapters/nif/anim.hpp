#ifndef VSGOPENMW_VSGADAPTERS_NIF_ANIM_H
#define VSGOPENMW_VSGADAPTERS_NIF_ANIM_H

#include <optional>

#include <components/animation/interpolate.hpp>
#include <components/animation/extrapolate.hpp>
#include <components/animation/constant.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/nif/nifkey.hpp>
#include <components/nif/data.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/node.hpp>
#include <components/nif/extra.hpp>

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
    inline vsg::ref_ptr<Channel> handleKeyData(const T &src)
    {
        auto ret = vsg::ref_ptr{new Channel};
        for (auto &[key, val] : src.mKeys)
            ret->keys[key] = toVsg(val.mValue);
        return ret;
    }

    template <class Channel, class T>
    inline vsg::ref_ptr<Channel> handleQuadraticKeyData(const T &src)
    {
        auto ret = vsg::ref_ptr{new Channel};
        for (auto &[key, val] : src.mKeys)
            ret->keys[key] = {toVsg(val.mValue), toVsg(val.mInTan), toVsg(val.mOutTan)};
        return ret;
    }

    template<>
    inline vsg::ref_ptr<Anim::CubicHermiteSpline<vsg::quat>> handleQuadraticKeyData(const Nif::QuaternionKeyMap &src)
    {
        return handleKeyData<Anim::CubicHermiteSpline<vsg::quat>>(src);
    }

    inline Anim::ExtrapolationMode convertExtrapolationMode(int mode)
    {
        switch(mode)
        {
        case Nif::Controller::ExtrapolationMode::Cycle: return Anim::ExtrapolationMode::Cycle;
        case Nif::Controller::ExtrapolationMode::Reverse: return Anim::ExtrapolationMode::Reverse;
        case Nif::Controller::ExtrapolationMode::Constant:
        default: return Anim::ExtrapolationMode::Constant;
        }
    }

    template<class T>
    inline void addExtrapolatorIfRequired(const Nif::Controller &ctrl, Anim::channel_ptr<T> &channel, float startKey, float stopKey)
    {
        Anim::ExtrapolationMode mode = convertExtrapolationMode((ctrl.flags&0x6)>>1);

        float phase = ctrl.phase;
        if (0)//if (ctrl.flags & Nif::NiNode::AnimFlag_Random)
            phase = std::rand()/static_cast<float>(RAND_MAX);
        if (ctrl.frequency == 1 && phase == 0 && mode == Anim::ExtrapolationMode::Constant && ctrl.timeStart <= startKey && ctrl.timeStop >= stopKey)
            return;

        if (ctrl.timeStop - ctrl.timeStart <= 0)
        {
            channel = vsg::ref_ptr{new Anim::Constant<T>(channel->value(ctrl.timeStart))};
            return;
        }

        auto settings = Anim::ExtrapolationSettings{ctrl.frequency, ctrl.phase, ctrl.timeStart, ctrl.timeStop};
        if (mode == Anim::ExtrapolationMode::Constant)
           channel = vsg::ref_ptr{new Anim::Extrapolate<T, Anim::ExtrapolationMode::Constant>(channel, settings)};
        else if (mode == Anim::ExtrapolationMode::Reverse)
           channel = vsg::ref_ptr{new Anim::Extrapolate<T, Anim::ExtrapolationMode::Reverse>(channel, settings)};
        else if (mode == Anim::ExtrapolationMode::Cycle)
           channel = vsg::ref_ptr{new Anim::Extrapolate<T, Anim::ExtrapolationMode::Cycle>(channel, settings)};
    }

    template <class T, class Src>
    inline Anim::channel_ptr<T> handleInterpolatedKeyframes(const Src &src)
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
    inline Anim::channel_ptr<T> handleKeyframes(const Nif::Controller &ctrl, const Src &src, const std::optional<T> defaultValue = {})
    {
        if (!src || src->mKeys.empty())
        {
            if (defaultValue)
                return vsg::ref_ptr{new Anim::Constant(*defaultValue)};
            return {};
        }
        if (src->mKeys.size() == 1)
            return vsg::ref_ptr{new Anim::Constant(toVsg(src->mKeys.begin()->second.mValue))};

        auto ret = handleInterpolatedKeyframes<T>(src);
        float startKey = src->mKeys.begin()->first;
        float stopKey = src->mKeys.rbegin()->first;
        addExtrapolatorIfRequired(ctrl, ret, startKey, stopKey);
        return ret;
    }

    void convertTrafo(Anim::Transform &trans, const Nif::Transformation &trafo);

    vsg::ref_ptr<Anim::Tags> handleTextKeys(const Nif::NiTextKeyExtraData &keys);

    vsg::ref_ptr<Anim::TransformController> handleKeyframeController(const Nif::NiKeyframeController &keyctrl);
    vsg::ref_ptr<Anim::TransformController> handlePathController(const Nif::NiPathController &pathctrl);
    vsg::ref_ptr<Anim::Roll> handleRollController(const Nif::NiRollController &rollctrl);
}

#endif
