#include "object.hpp"

#include <components/animation/transform.hpp>

#include "play.hpp"

namespace MWAnim
{
    Object::Object(const Context &ctx)
        : mContext(ctx)
    {
        mTransform = new Anim::Transform;
    }

    Object::~Object()
    {
    }

    vsg::ref_ptr<vsg::Node> Object::node()
    {
        return mTransform;
    }

    std::vector<const Anim::Transform*> Object::headTransform() const
    {
        auto node = worldTransform("Head");
        if (node.empty())
            node = worldTransform("Bip01 Head");
        return node;
    }

    Anim::Bone *Object::searchBone(const std::string &name)
    {
        if (animation)
            if (auto bone = animation->bones.search(name))
                return bone;
        return {};
    }

    std::vector<const Anim::Transform*> Object::worldTransform(const std::string &name) const
    {
        if (auto bone = const_cast<Object*>(this)->searchBone(name))
            return worldTransform(*bone);
        return {};
    }

    std::vector<const Anim::Transform*> Object::worldTransform(const Anim::Bone &bone) const
    {
        std::vector<const Anim::Transform*> path;
        path.reserve(bone.path.size()+1);
        path.push_back(mTransform);
        std::copy(bone.path.begin(), bone.path.end(), std::back_inserter(path));
        return path;
    }
}
