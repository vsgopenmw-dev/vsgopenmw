#ifndef VSGOPENMW_MWANIMATION_CLONE_H
#define VSGOPENMW_MWANIMATION_CLONE_H

#include <string>

#include <vsg/core/ref_ptr.h>

#include <components/animation/animation.hpp>
#include <components/animation/contents.hpp>

namespace vsg
{
    class Node;
    class Group;
    class Descriptor;
}
namespace Anim
{
    class Transform;
}
namespace MWAnim
{
    struct Placeholders
    {
        vsg::Group* attachLight{};
        Anim::Transform* attachAmmo{};
        const Anim::Transform* boneOffset{};
    };

    struct CloneResult : public Anim::Animation
    {
        Anim::Contents contents;
    };
    struct PlaceholderResult : public CloneResult
    {
        Placeholders placeholders;
    };

    /*
     * Implements clone callbacks.
     */
    CloneResult cloneIfRequired(vsg::ref_ptr<vsg::Node>& node);
    PlaceholderResult clonePlaceholdersIfRequired(vsg::ref_ptr<vsg::Node>& node);
    CloneResult cloneIfRequired(vsg::ref_ptr<vsg::Node>& node, const std::string& removeNodesStartingWith);
    CloneResult cloneAndReplace(vsg::ref_ptr<vsg::Node>& node, vsg::ref_ptr<vsg::Descriptor> texture = {},
        bool overrideAllTextures = true, const std::vector<vsg::ref_ptr<vsg::Node>>& replaceDummyNodesWith = {});
}

#endif
