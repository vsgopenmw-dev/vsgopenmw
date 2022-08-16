#include "clone.hpp"

#include <sstream>

#include <vsg/state/DescriptorBuffer.h>
#include <vsg/state/BindDynamicDescriptorSet.h>

#include <components/vsgutil/name.hpp>
#include <components/animation/transform.hpp>
#include <components/animation/meta.hpp>
#include <components/misc/strings/algorithm.hpp>

namespace
{
    inline vsg::ref_ptr<vsg::Node> getPlaceholders(const vsg::Node &n, MWAnim::Placeholders &placeholders)
    {
        if (!n.getAuxiliary())
            return {};

        const auto &name = vsgUtil::getName(n);
        if (Misc::StringUtils::ciEqual(name, "AttachLight"))
        {
            auto transform = static_cast<const Anim::Transform*>(&n);
            auto ret = vsg::ref_ptr{new Anim::Transform(*transform)};
            placeholders.attachLight = ret.get();
            return ret;
        }
        else if (Misc::StringUtils::ciEqual(name, "BoneOffset"))
        {
            placeholders.boneOffset = static_cast<const Anim::Transform*>(&n);
            /*removeNode()*/return vsg::Node::create();
        }
        else if (Misc::StringUtils::ciEqual(name, "ArrowBone"))
        {
            auto transform = static_cast<const Anim::Transform*>(&n);
            auto ret = vsg::ref_ptr{new Anim::Transform(*transform)};
            placeholders.attachAmmo = ret.get();
            return ret;
        }
        return {};
    }

    vsg::ref_ptr<vsg::Node> overrideTexture(const vsg::Node &n, vsg::ref_ptr<vsg::Descriptor> texture)
    {
        if (auto *bds = dynamic_cast<const vsg::BindDescriptorSet*>(&n))
        {
            if (bds->pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS)
                return {};
            auto set = bds->descriptorSet;
            for (size_t i=0; i<set->descriptors.size(); ++i)
            {
                auto &descriptor = set->descriptors[i];
                if (descriptor->dstBinding == texture->dstBinding)
                {
                    auto newDescriptors = set->descriptors;
                    newDescriptors[i] = texture;
                    auto newDescriptorSet = vsg::DescriptorSet::create(set->setLayout, newDescriptors);
                    vsg::ref_ptr<vsg::BindDescriptorSet> newBds;
                    if (dynamic_cast<const vsg::BindDynamicDescriptorSet*>(bds))
                        newBds = vsg::BindDynamicDescriptorSet::create(bds->pipelineBindPoint, bds->layout, bds->firstSet, newDescriptorSet);
                    else
                        newBds = vsg::BindDescriptorSet::create(bds->pipelineBindPoint, bds->layout, bds->firstSet, newDescriptorSet);
                    return newBds;
                }
            }
        }
        return {};
    }
}

namespace MWAnim
{
    PlaceholderResult clonePlaceholdersIfRequired(vsg::ref_ptr<vsg::Node> &node)
    {
        PlaceholderResult ret;
        ret.contents = Anim::Meta::getContents(*node);
        if (ret.contents.contains(Anim::Contents::Controllers|Anim::Contents::Placeholders))
        {
            Anim::CloneCallback callback = [&ret] (auto &o) -> auto { return getPlaceholders(o, ret.placeholders); };
            static_cast<Anim::CloneResult&>(ret) = Anim::cloneIfRequired(node, callback);
        }
        return ret;
    }

    CloneResult cloneIfRequired(vsg::ref_ptr<vsg::Node> &node)
    {
        CloneResult ret{.contents=Anim::Meta::getContents(*node)};
        if (ret.contents.contains(Anim::Contents::Controllers))
            static_cast<Anim::CloneResult&>(ret) = Anim::cloneIfRequired(node);
        return ret;
    }

    CloneResult cloneIfRequired(vsg::ref_ptr<vsg::Node> &node, const std::string &nodeToRemove)
    {
        auto callback = [&nodeToRemove] (const vsg::Node &n) -> vsg::ref_ptr<vsg::Node> {
            if (Misc::StringUtils::ciCompareLen(vsgUtil::getName(n), nodeToRemove, nodeToRemove.size()) == 0)
                return vsg::Node::create();
            return {};
        };
        CloneResult ret{.contents=Anim::Meta::getContents(*node)};
        static_cast<Anim::CloneResult&>(ret) = Anim::cloneIfRequired(node, callback);
        return ret;
    }

    CloneResult cloneAndReplace(vsg::ref_ptr<vsg::Node> &node, vsg::ref_ptr<vsg::Descriptor> texture, bool overrideAll, const std::vector<vsg::ref_ptr<vsg::Node>> &replaceDummyNodesWith)
    {
        CloneResult ret{.contents=Anim::Meta::getContents(*node)};
        bool overrideActive = true;
        auto callback = [&] (const vsg::Node &n) -> vsg::ref_ptr<vsg::Node> {
            if (overrideActive)
            {
                auto replaced = overrideTexture(n, texture);
                if (replaced)
                {
                    if (!overrideAll)
                        overrideActive = false;
                    return replaced;
                }
            }

            auto &name = vsgUtil::getName(n);
            std::string_view dummy = "Dummy";
            if (Misc::StringUtils::ciCompareLen(name, dummy, dummy.size()) == 0 && dynamic_cast<const vsg::Group*>(&n))
            {
                std::stringstream stream;
                int id = 0;
                stream << name.substr(dummy.size(), 2);
                stream >> id;
                if (id >= 0 && id < static_cast<int>(replaceDummyNodesWith.size()))
                {
                    auto &r = replaceDummyNodesWith[id];
                    ret.contents.contents |= Anim::Meta::getContents(*r).contents;
                    return r;
                }
            }
            return {};
        };
        static_cast<Anim::CloneResult&>(ret) = Anim::cloneIfRequired(node, callback);
        return ret;
    }
}
