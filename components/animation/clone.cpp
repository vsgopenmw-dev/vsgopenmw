#include "clone.hpp"

#include <iostream>

#include <vsg/nodes/CullGroup.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/DepthSorted.h>
#include <vsg/nodes/LOD.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/Switch.h>
#include <vsg/nodes/Transform.h>
#include <vsg/commands/Commands.h>
#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorBuffer.h>
#include <vsg/state/StateSwitch.h>

#include "billboard.hpp"
#include "controller.hpp"

namespace vsg
{
    bool operator==(const Switch::Child& lhs, const Switch::Child& rhs)
    {
        return lhs.node == rhs.node;
    }
    bool operator==(const StateSwitch::Child& lhs, const StateSwitch::Child& rhs)
    {
        return lhs.stateCommand == rhs.stateCommand;
    }
}

namespace
{
    /*
     * Speeds up visitor.
     */
    struct Null
    {
        constexpr vsg::Object* operator()(const vsg::Object&) { return {}; }
    };
}

namespace Anim
{
    template <class Callback>
    class Clone : public vsg::ConstVisitor
    {
    public:
        Callback callback;
        Animation& objects;
        Clone(Callback c, Animation& objs)
            : callback(c)
            , objects(objs)
        {
            overrideMask = vsg::MASK_ALL;
        }
        vsg::ref_ptr<vsg::Object> cloned;

        template <class Obj>
        vsg::ref_ptr<Obj> clone(const Obj& obj)
        {
            resetAndAccept(obj);
            return vsg::ref_ptr{ static_cast<Obj*>(cloned.get()) };
        }

    private:
        std::map<const vsg::Object*, vsg::ref_ptr<vsg::Object>> mClonedObjects;
        void resetAndAccept(const vsg::Object& obj)
        {
            cloned = nullptr;
            obj.accept(*this);
        }
        void resetAndAccept(const vsg::Node& n)
        {
            if (auto result = callback(n))
                cloned = result;
            else
                resetAndAccept(static_cast<const vsg::Object&>(n));
        }
        void cloneController(vsg::Object& o)
        {
            if (auto ctrl = Controller::get(o))
            {
                if (auto copyCtrl = ctrl->cloneIfRequired())
                {
                    objects.clonedControllers.emplace_back(copyCtrl, &o);
                    copyCtrl->attachTo(o);
                }
                else
                    objects.controllers.emplace_back(ctrl, &o);
            }
        }
        void copyAuxiliary(const vsg::Object& src, vsg::Object& dst)
        {
            if (src.getAuxiliary())
            {
                dst.getOrCreateAuxiliary()->userObjects = src.getAuxiliary()->userObjects;
                cloneController(dst);
            }
        }

        void warn(const vsg::Object& obj) { std::cerr << "!clone(" << typeid(obj).name() << ")" << std::endl; }
        void apply(const vsg::Object& obj) override { warn(obj); }

        void apply(const vsg::Data& d) override
        {
            //if (!Controller::get(d))
            if (!d.dynamic() && !Controller::get(d))
                return;
            auto found = mClonedObjects.find(&d);
            if (found != mClonedObjects.end())
                cloned = found->second;
            else
            {
                cloned = d.clone();
                copyAuxiliary(d, *cloned);
                mClonedObjects[&d] = cloned;
            }
        }

        void apply(const vsg::Draw&) override {}
        void apply(const vsg::VertexIndexDraw&) override {}
        void apply(const vsg::Command&) override {}
        void apply(const vsg::BindGraphicsPipeline&) override {}
        void apply(const vsg::BindComputePipeline&) override {}
        void apply(const vsg::BindDescriptorSet& bds) override
        {
            if (auto set = cloneDescriptorSetIfRequired(*bds.descriptorSet))
                cloned = vsg::BindDescriptorSet::create(bds.pipelineBindPoint, bds.layout, bds.firstSet, set);
        }
        vsg::ref_ptr<vsg::DescriptorSet> cloneDescriptorSetIfRequired(const vsg::DescriptorSet& set)
        {
            auto descriptors = cloneChildrenIfRequired(set.descriptors);
            if (descriptors != set.descriptors)
                return vsg::DescriptorSet::create(set.setLayout, descriptors);
            return {};
        }
        void apply(const vsg::Descriptor& d) override {}
        void apply(const vsg::DescriptorImage& d) override {}
        void apply(const vsg::DescriptorBuffer& d) override
        {
            resetAndAccept(*d.bufferInfoList[0]->data);
            if (!cloned)
                return;
            auto found = mClonedObjects.find(&d);
            if (found != mClonedObjects.end())
                cloned = found->second;
            else
            {
                cloned = vsg::DescriptorBuffer::create(
                    vsg::ref_ptr{ static_cast<vsg::Data*>(cloned.get()) }, d.dstBinding, d.dstArrayElement, d.descriptorType);
                mClonedObjects[&d] = cloned;
            }
        }

        void apply(const vsg::Commands& group) override { applyGroup<false>(group); }
        void apply(const vsg::Group& group) override { applyGroup<false>(group); }
        void apply(const vsg::CullGroup& group) override { applyGroup<false>(group); }
        void apply(const vsg::Switch& group) override { applyGroup<true>(group); }
        void apply(const vsg::StateSwitch& group) override { applyGroup<true>(group); }

        void apply(const vsg::CullNode& node) override { applyNodeWithChild(node); }
        void apply(const vsg::DepthSorted& node) override { applyNodeWithChild(node); }

        void apply(const vsg::LOD& group) override { warn(group); }

        void apply(const vsg::Transform& group) override
        {
            if (auto t = dynamic_cast<const Anim::Transform*>(&group))
                applyGroup<true>(*t);
            else if (auto* b = dynamic_cast<const Anim::Billboard*>(&group))
                applyGroup<true>(*b);
            else
                warn(group);
        }

        void apply(const vsg::StateGroup& group) override
        {
            auto children = cloneChildrenIfRequired(group.children);
            auto stateCommands = cloneChildrenIfRequired(group.stateCommands);
            if (children != group.children || stateCommands != group.stateCommands)
            {
                cloneGroup(group, children);
                static_cast<vsg::StateGroup*>(cloned.get())->stateCommands = stateCommands;
            }
        }

        template <class Node>
        void applyNodeWithChild(const Node& node)
        {
            if (auto child = cloneChildIfRequired(node.child))
            {
                auto ret = copy(node);
                ret->child = child;
                cloned = ret;
            }
        }

        template <bool Controllable, class Group>
        void applyGroup(const Group& group)
        {
            auto children = cloneChildrenIfRequired(group.children);
            {
                cloneGroup(group, children);
                if (Controllable)
                    cloneController(*cloned);
            }
        }

        template <class Group, class Children>
        void cloneGroup(const Group& group, const Children& children)
        {
            auto ret = vsg::ref_ptr{ new Group(group) };
            ret->children = children;
            cloned = ret;
        }

        template <class Children>
        Children cloneChildrenIfRequired(const Children& children)
        {
            Children ret = children;
            for (size_t i = 0; i < children.size(); ++i)
            {
                auto c = cloneChildIfRequired(children[i]);
                if (valid(c))
                    ret[i] = c;
            }
            return ret;
        }

        template <class T>
        vsg::ref_ptr<T> copy(const T& t)
        {
            return vsg::ref_ptr{ new T(t) };
        }

        template <class Obj>
        vsg::ref_ptr<Obj> cloneChildIfRequired(vsg::ref_ptr<Obj> n)
        {
            resetAndAccept(*n);
            return vsg::ref_ptr{ static_cast<Obj*>(cloned.get()) };
        }
        vsg::Switch::Child cloneChildIfRequired(const vsg::Switch::Child& c)
        {
            return { c.mask, cloneChildIfRequired(c.node) };
        }
        vsg::StateSwitch::Child cloneChildIfRequired(const vsg::StateSwitch::Child& c)
        {
            return { c.mask, cloneChildIfRequired(c.stateCommand) };
        }

        template <class Obj>
        bool valid(vsg::ref_ptr<Obj> obj)
        {
            return obj.valid();
        }
        bool valid(const vsg::Switch::Child& c) { return c.node.valid(); }
        bool valid(const vsg::StateSwitch::Child& c) { return c.stateCommand.valid(); }
    };

    Animation cloneIfRequired(vsg::ref_ptr<vsg::Node>& node, CloneCallback f)
    {
        Animation ret;
        if (f)
        {
            auto visitor = Clone(f, ret);
            if (auto cloned = visitor.clone(*node))
                node = cloned;
        }
        else
        {
            auto visitor = Clone(Null(), ret);
            if (auto cloned = visitor.clone(*node))
                node = cloned;
        }
        return ret;
    }
}
