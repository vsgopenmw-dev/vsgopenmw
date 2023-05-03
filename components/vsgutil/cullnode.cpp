#include "cullnode.hpp"

#include <iostream>
#include <stack>

#include <vsg/io/stream.h>
#include <vsg/core/visit.h>
#include <vsg/nodes/DepthSorted.h>
#include <vsg/nodes/CullGroup.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/Switch.h>
#include <vsg/nodes/LOD.h>
#include <vsg/nodes/Transform.h>
#include <vsg/utils/ComputeBounds.h>

#include "bounds.hpp"
#include "group.hpp"

//#define CHECK_CULL_NODES

namespace vsgUtil
{
    class ComputeAllBounds : public vsg::ComputeBounds
    {
    public:
        ComputeAllBounds() { overrideMask = vsg::MASK_ALL; }
    };

    vsg::dsphere getBounds(vsg::ref_ptr<vsg::Node> node)
    {
        return toSphere(vsg::visit<ComputeAllBounds>(node).bounds);
    }

    vsg::ref_ptr<vsg::Node> createCullNode(vsg::ref_ptr<vsg::Node> child)
    {
        auto bounds = getBounds(child);
        if (!bounds.valid())
            return child;
        return vsg::CullNode::create(bounds, child);
    }

    vsg::ref_ptr<vsg::Node> createCullNode(vsg::Group::Children& children)
    {
        if (children.size() == 1)
            return createCullNode(children[0]);
        auto bounds = vsg::visit<ComputeAllBounds>(children).bounds;
        if (!bounds.valid())
            return moveChildren<vsg::Group>(children);
        return moveChildren<vsg::CullGroup>(children, toSphere(bounds));
    }

#ifdef CHECK_CULL_NODES
    class CheckCullNodes : public vsg::Visitor
    {
    public:
        bool good = true;
        CheckCullNodes() { overrideMask = vsg::MASK_ALL; }
        void apply(vsg::Node& n) override { n.traverse(*this); }
        void apply(vsg::CullNode&) override {}
        void apply(vsg::CullGroup&) override {}
        void apply(vsg::StateGroup& group) override
        {
            if (!group.prototypeArrayState)
            {
                auto bounds = vsg::visit<ComputeAllBounds>(&group).bounds;
                good = false;
                std::cerr << "!CheckCullNodes(" << bounds.min << " " << bounds.max << ")" << std::endl;
                group.traverse(*this);
            }
        }
        void apply(vsg::VertexIndexDraw& group) override
        {
            std::cerr << "(VertexIndexDraw)" << std::endl;
        }
    };
#endif

    class AddLeafCullNodes : public vsg::Visitor
    {
        std::stack<vsg::Transform*> mTransformStack;

    public:
        bool dryRun = false;
        int numCullNodes = 0;
        vsg::ref_ptr<vsg::Node> replaced;

        AddLeafCullNodes() { overrideMask = vsg::MASK_ALL; }
        void apply(vsg::Node& n) override
        {
            std::cerr << "!AddLeafCullNodes<" << typeid(n).name() << ">" << std::endl;
        }
        void apply(vsg::DepthSorted& node) override
        {
            if (!mTransformStack.empty())
                mTransformStack.top()->subgraphRequiresLocalFrustum = true;
            //if (intersector_supports_depthsorted_culling && bound.valid()) return;
            replaceChild(node.child);
        }
        void apply(vsg::CullNode& node) override
        {
            replaceChild(node.child);
        }
        void apply(vsg::CullGroup& node) override
        {
            replaceChildren(node.children);
        }
        void apply(vsg::Transform& t) override
        {
            mTransformStack.push(&t);
            applyGroup(t);
            mTransformStack.pop();
        }
        void apply(vsg::StateGroup& group) override
        {
            if (!group.prototypeArrayState)
            {
                if (dryRun)
                    ++numCullNodes;
                else
                {
                    if (!mTransformStack.empty())
                        mTransformStack.top()->subgraphRequiresLocalFrustum = true;
                    auto created = createCullNode(vsg::ref_ptr{ &group });
                    if (created != &group)
                        replaced = created;
                }
            }
        }
        void apply(vsg::Group& group) override { applyGroup(group); }
        void apply(vsg::Switch& group) override { applyGroup(group); }
        void apply(vsg::LOD& group) override { applyGroup(group); }

        template <class Group>
        void applyGroup(Group& group)
        {
            replaceChildren(group.children);
        }

        template <class Children>
        void replaceChildren(Children& children)
        {
            for (auto& child : children)
                replaceChild(child);
        }

        void replaceChild(vsg::ref_ptr<vsg::Node>& n)
        {
            n->accept(*this);
            if (replaced)
            {
                n = replaced;
                replaced = {};
            }
        }
        void replaceChild(vsg::Switch::Child& c) { replaceChild(c.node); }
        void replaceChild(vsg::LOD::Child& c) { replaceChild(c.node); }
    };

    void addLeafCullNodes(vsg::ref_ptr<vsg::Node>& n, int minCount)
    {
        AddLeafCullNodes visitor;
        if (minCount > 1)
        {
            visitor.dryRun = true;
            n->accept(visitor);
            if (visitor.numCullNodes < minCount)
                return;
            visitor.dryRun = false;
        }
        n->accept(visitor);
        if (visitor.replaced) // if (n->cast<vsg::StateGroup>())
            n = visitor.replaced;

#ifdef CHECK_CULL_NODES
        CheckCullNodes check;
        n->accept(check);
        if (!check.good)
            std::cerr << "(" << typeid(*n).name() << ")" << std::endl;
#endif
    }
}
