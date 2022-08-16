#include "cullnode.hpp"

#include <stack>

#include <vsg/traversals/ComputeBounds.h>
#include <vsg/nodes/Transform.h>
#include <vsg/nodes/Switch.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/CullGroup.h>
#include <vsg/core/visit.h>

#include "group.hpp"
#include "box.hpp"

namespace vsgUtil
{
    inline vsg::ComputeBounds computeAllBounds()
    {
        auto visitor = vsg::ComputeBounds();
        visitor.overrideMask = vsg::MASK_ALL;
        return visitor;
    }

    vsg::ref_ptr<vsg::Node> createCullNode(vsg::Node &child)
    {
        auto visitor = computeAllBounds();
        child.accept(visitor);
        return vsg::CullNode::create(toSphere(visitor.bounds), &child);
    }

    vsg::ref_ptr<vsg::Node> createCullNode(vsg::Group::Children &children)
    {
        if (children.size()==1)
            return createCullNode(*children[0]);
        auto visitor = computeAllBounds();
        auto bounds = vsg::visit(visitor, children).bounds;
        return moveChildren<vsg::CullGroup>(children, toSphere(bounds));
    }

    class AddLeafCullNodes : public vsg::Visitor
    {
        vsg::ref_ptr<vsg::Node> mReplaced;
        std::stack<vsg::Transform*> mTransformStack;
    public:
        AddLeafCullNodes()
        {
            overrideMask = vsg::MASK_ALL;
        }
        void apply(vsg::Node &n)
        {
            n.traverse(*this);
        }
        void apply(vsg::DepthSorted&) {}
        void apply(vsg::CullNode&) {}
        void apply(vsg::CullGroup&) {}
        void apply(vsg::Transform &t)
        {
            mTransformStack.push(&t);
            applyGroup(t);
            mTransformStack.pop();
        }
        void apply(vsg::StateGroup &group)
        {
            if (!group.prototypeArrayState)
            {
                if (!mTransformStack.empty())
                    mTransformStack.top()->subgraphRequiresLocalFrustum = true;
                mReplaced = createCullNode(group);
            }
            //else
            //vsgopenmw-dynamic-culling
        }
        void apply(vsg::Group &group)
        {
            applyGroup(group);
        }
        void apply(vsg::Switch &group)
        {
            applyGroup(group);
        }
        /*
        void apply(const vsg::LOD &group) override
        {
            applyGroup(group);
        }
        */

        template <class Group>
        void applyGroup(Group &group)
        {
            replaceChildren(group.children);
        }

        template <class Children>
        void replaceChildren(Children &children)
        {
            for (auto &child : children)
                replaceChild(child);
        }

        void replaceChild(vsg::ref_ptr<vsg::Node> &n)
        {
            n->accept(*this);
            if (mReplaced)
            {
                n = mReplaced;
                mReplaced = {};
            }
        }
        void replaceChild(vsg::Switch::Child &c)
        {
            replaceChild(c.node);
        }
    };

    void addLeafCullNodes(vsg::Node &n)
    {
        AddLeafCullNodes visitor;
        n.accept(visitor);
    }
}
