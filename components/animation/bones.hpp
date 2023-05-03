#ifndef VSGOPENMW_ANIMATION_BONES_H
#define VSGOPENMW_ANIMATION_BONES_H

#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

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
        Transform* node() { return path.back(); }
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
        /*
         * Specifies whether bone nodes are attached to each other in the scene graph.
         * @note Attaching only required bones may allow for a smaller scene graph.
         */
        const bool attached;
        using NormalizeFunc = std::function<std::string(const std::string&)>;
        /*
         * Creates detached copy of bones from provided graph.
         */
        Bones(NormalizeFunc f, const vsg::Node& graphToCopy, const std::map<std::string, uint32_t>& groups = {});

        /*
         * Reads attached bones from provided graph.
         */
        Bones(NormalizeFunc f, vsg::Node& scene);
        /*
         * Optionally supports case-insensitive searches.
         */
        NormalizeFunc normalizeFunc = [](auto& s) { return s; };

        Bone* search(const std::string& name);
    };
}

#endif
