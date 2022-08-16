#ifndef VSGOPENMW_MWANIMATION_CLONE_H
#define VSGOPENMW_MWANIMATION_CLONE_H

#include <string>

#include <vsg/core/ref_ptr.h>

#include <components/animation/contents.hpp>
#include <components/animation/clone.hpp>

namespace vsg
{
    class Node;
    class Group;
}
namespace Anim
{
    class Transform;
}
namespace MWAnim
{
    struct Placeholders
    {
        vsg::Group *attachLight{};
        vsg::Group *attachAmmo{};
        const Anim::Transform *boneOffset{};
    };

    struct CloneResult : public Anim::CloneResult
    {
        Anim::Contents contents;
    };
    struct PlaceholderResult : public CloneResult
    {
        Placeholders placeholders;
    };

    /*
     * Clones selectively.
     */
    CloneResult cloneIfRequired(vsg::ref_ptr<vsg::Node> &node);
    PlaceholderResult clonePlaceholdersIfRequired(vsg::ref_ptr<vsg::Node> &node);
    CloneResult cloneIfRequired(vsg::ref_ptr<vsg::Node> &node, const std::string &removeNodesStartingWith);
    CloneResult cloneAndReplace(vsg::ref_ptr<vsg::Node> &node, vsg::ref_ptr<vsg::Descriptor> texture, bool overrideAllTextures=true, const std::vector<vsg::ref_ptr<vsg::Node>> &replaceDummyNodesWith={});
}

#endif
