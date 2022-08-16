#ifndef VSGOPENMW_MWANIMATION_OBJECT_H
#define VSGOPENMW_MWANIMATION_OBJECT_H

#include <memory>
#include <vector>

#include <vsg/core/ref_ptr.h>
#include <vsg/maths/vec3.h>

#include <components/animation/update.hpp>

namespace vsg
{
    class Node;
}
namespace Anim
{
    class Transform;
    class Bone;
}
namespace MWAnim
{
    class Play;
    class Context;

    /*
     * Contains animation scene graph.
     */
    class Object
    {
    protected:
        vsg::ref_ptr<Anim::Transform> mTransform;
        const Context &mContext;
    public:
        Object(const Context &ctx);
        virtual ~Object();

        const Context &context() const { return mContext; }

        virtual void update(float dt) {}

        Anim::AutoUpdate autoPlay;

        std::unique_ptr<Play> /*optional_*/animation;

        // Returns a bone's world transform or {}.
        std::vector<const Anim::Transform*> worldTransform(const std::string &bone) const;
        // Returns a bone's world transform.
        std::vector<const Anim::Transform*> worldTransform(const Anim::Bone &bone) const;

        std::vector<const Anim::Transform*> headTransform() const;

        Anim::Bone *searchBone(const std::string &name);

        Anim::Transform *transform() { return mTransform.get(); }
        const Anim::Transform *transform() const { return mTransform.get(); }
        vsg::ref_ptr<vsg::Node> node();
    };
}

#endif
