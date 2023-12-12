#ifndef VSGOPENMW_MWANIMATION_OBJECT_H
#define VSGOPENMW_MWANIMATION_OBJECT_H

#include <memory>
#include <vector>

#include <vsg/core/ref_ptr.h>
#include <vsg/maths/vec3.h>

#include <components/animation/autoplay.hpp>

namespace vsg
{
    class Node;
    class Group;
    class StateGroup;
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
     * Object is a container class for an animation scene graph.
     * Several important members are public, this is a conscious choice to allow the scene graph to be extended through composition, and to avoid the Object class hierarchy from being cluttered with rarely used features like extra lights, effects, decorations and transparency etc.
     */
    class Object
    {
    protected:
        vsg::ref_ptr<vsg::StateGroup> mStateGroup;
        vsg::ref_ptr<Anim::Transform> mTransform;
        const Context& mContext;

    public:
        Object(const Context& ctx);
        virtual ~Object();

        const Context& context() const { return mContext; }

        virtual void update(float dt)
        {
            autoPlay.update(dt);
        }

        Anim::AutoPlay autoPlay;

        inline void addController(const Anim::Controller* ctrl, vsg::Object* target)
        {
            if (ctrl->hints.autoPlay)
                autoPlay.add(ctrl, target);
        }

        std::unique_ptr<Play> /*optional_*/ animation;
        inline void assignBones() {}

        // Returns a bone's world transform or {}.
        std::vector<const Anim::Transform*> worldTransform(const std::string& bone) const;
        // Returns a bone's world transform.
        std::vector<const Anim::Transform*> worldTransform(const Anim::Bone& bone) const;

        std::vector<const Anim::Transform*> headTransform() const;

        Anim::Bone* searchBone(const std::string& name);

        /*
         * The transform node is used to attach and place the object within the scene, and is a member of the Object class for convenience.
         */
        Anim::Transform* transform() { return mTransform.get(); }
        const Anim::Transform* transform() const { return mTransform.get(); }

        /*
         * node() returns the topmost node that should be used to attach the Object to the scene graph, currently the transform() node.
         * It is expected to remain for the lifetime of the Object and can also be used to attach metadata to its auxiliary container.
         */
        vsg::ref_ptr<vsg::Node> node();

        /*
         * An optional StateGroup is provided for extensions. When a StateGroup is created, existing children are re-parented to it.
         */
        vsg::StateGroup* getOrCreateStateGroup();
        vsg::StateGroup* getStateGroup();
        /*
         * Returns the node that children should be added to and removed from. This node will change if/when a StateGroup is created.
         */
        vsg::Group* nodeToAddChildrenTo();
    };
}

#endif
