#ifndef VSGOPENMW_ANIMATION_BONES_H
#define VSGOPENMW_ANIMATION_BONES_H

#include <vector>
#include <map>
#include <string>
#include <unordered_map>
#include <functional>

#include <vsg/core/ref_ptr.h>

namespace vsg
{
    class Node;
}

namespace Anim
{
    class Transform;
    using BonePath = std::vector<vsg::ref_ptr<Transform>>;
    struct Bone
    {
        BonePath path;
        uint32_t group = 0;
        Transform *node() { return path.back(); }
    };

    /*
     * Maps a transform hierarchy not necessarily attached to the scene graph.
     */
    class Bones
    {
        friend class InitBones;
        friend class CopyBones;
        std::unordered_map<std::string, Bone> mByName;
    public:
        Bones();
        ~Bones();
        using NormalizeFunc = std::function<std::string(const std::string&)>;
        Bones(NormalizeFunc f, const vsg::Node &graphToCopy, const std::map<std::string, uint32_t> &groups = {});
        Bones(NormalizeFunc f, vsg::Node &scene);
        NormalizeFunc normalizeFunc = [](auto &s){return s;};
        Bone *search(const std::string &name);
    };
}

#endif
