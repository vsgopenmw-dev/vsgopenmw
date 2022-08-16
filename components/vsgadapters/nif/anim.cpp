#include "anim.hpp"

#include <components/animation/rotate.hpp>
#include <components/animation/tags.hpp>
#include <components/animation/roll.hpp>
#include <components/animation/transformcontroller.hpp>
#include <components/animation/path.hpp>

#include <components/misc/strings/lower.hpp>
#include <components/misc/strings/algorithm.hpp>

namespace vsgAdapters
{
    std::vector<Anim::Rotate::AngleAxis> orderRotations(const std::vector<Anim::Rotate::AngleAxis> &in, Nif::NiKeyframeData::AxisOrder order)
    {
        switch (order)
        {
        case Nif::NiKeyframeData::AxisOrder::Order_XYZ:
            return {in[0], in[1], in[2]};
        case Nif::NiKeyframeData::AxisOrder::Order_XZY:
            return {in[0], in[2], in[1]};
        case Nif::NiKeyframeData::AxisOrder::Order_YZX:
            return {in[1], in[2], in[0]};
        case Nif::NiKeyframeData::AxisOrder::Order_YXZ:
            return {in[1], in[0], in[2]};
        case Nif::NiKeyframeData::AxisOrder::Order_ZXY:
            return {in[2], in[0], in[1]};
        case Nif::NiKeyframeData::AxisOrder::Order_ZYX:
            return {in[2], in[1], in[0]};
        case Nif::NiKeyframeData::AxisOrder::Order_XYX:
            return {in[0], in[1], in[0]};
        case Nif::NiKeyframeData::AxisOrder::Order_YZY:
            return {in[1], in[2], in[1]};
        case Nif::NiKeyframeData::AxisOrder::Order_ZXZ:
            return {in[2], in[0], in[2]};
        }
    }

    Anim::channel_ptr<vsg::quat> handleXYZChannels(const Nif::NiKeyframeController &keyctrl)
    {
        const auto &data = *keyctrl.data.getPtr();
        auto xr = handleKeyframes<float>(keyctrl, data.mXRotations);
        auto yr = handleKeyframes<float>(keyctrl, data.mXRotations);
        auto zr = handleKeyframes<float>(keyctrl, data.mZRotations);
        if (!xr && !yr && !zr)
            return {};
        std::vector<Anim::Rotate::AngleAxis> rotations = {
            {xr, {1.f, 0.f, 0.f}},
            {yr, {0.f, 1.f, 0.f}},
            {zr, {0.f, 0.f, 1.f}}
        };
        rotations = orderRotations(rotations, data.mAxisOrder);
        auto rotate = vsg::ref_ptr{new Anim::Rotate};
        for (auto &[channel,axis] : rotations)
        {
            if (channel)
                rotate->rotations.emplace_back(channel,axis);
        }
        return rotate;
    }

    void convertTrafo(Anim::Transform &trans, const Nif::Transformation &trafo)
    {
        trans.translation = toVsg(trafo.pos);
        trans.setScale(trafo.scale);
        for (int i=0;i<3;++i)
            for (int j=0;j<3;++j)
                trans.rotation(j,i) = trafo.rotation.mValues[i][j];
    }

    vsg::ref_ptr<Anim::Tags> handleTextKeys(const Nif::NiTextKeyExtraData &keys)
    {
        auto tags = vsg::ref_ptr{new Anim::Tags};
        for(size_t i = 0;i < keys.list.size();i++)
        {
            std::vector<std::string> results;
            Misc::StringUtils::split(keys.list[i].text, results, "\r\n");
            for (std::string &result : results)
            {
                Misc::StringUtils::trim(result);
                Misc::StringUtils::lowerCaseInPlace(result);
                if (!result.empty())
                    tags->emplace(keys.list[i].time, std::move(result));
            }
        }
        return tags;
    }

    vsg::ref_ptr<Anim::TransformController> handleKeyframeController(const Nif::NiKeyframeController &keyctrl)
    {
        if (keyctrl.data.empty())
            return {};
        auto ctrl = vsg::ref_ptr{new Anim::TransformController};
        ctrl->scale = handleKeyframes<float>(keyctrl, keyctrl.data->mScales);
        ctrl->translate = handleKeyframes<vsg::vec3>(keyctrl, keyctrl.data->mTranslations);
        ctrl->rotate = handleKeyframes<vsg::quat>(keyctrl, keyctrl.data->mRotations);
        if (!ctrl->rotate)
            ctrl->rotate = handleXYZChannels(keyctrl);
        return ctrl;
    }

    vsg::ref_ptr<Anim::TransformController> handlePathController(const Nif::NiPathController &pathctrl)
    {
        if (pathctrl.posData.empty() || pathctrl.floatData.empty())
            return {};
        auto ctrl = vsg::ref_ptr{new Anim::TransformController};
        auto path = vsg::ref_ptr{new Anim::Path};
        path->path = handleKeyframes<vsg::vec3>(pathctrl, pathctrl.posData->mKeyList, {vsg::vec3()});
        path->percent = handleKeyframes<float>(pathctrl, pathctrl.floatData->mKeyList, {0.f});
        ctrl->translate = path;
        return ctrl;
    }

    vsg::ref_ptr<Anim::Roll> handleRollController(const Nif::NiRollController &rollctrl)
    {
        if (rollctrl.data.empty())
            return {};
        auto ctrl = vsg::ref_ptr{new Anim::Roll};
        ctrl->speed = handleKeyframes<float>(rollctrl, rollctrl.data->mKeyList);
        return ctrl;
    }
}
