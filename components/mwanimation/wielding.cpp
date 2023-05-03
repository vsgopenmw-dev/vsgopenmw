#include "wielding.hpp"

#include <components/animation/transform.hpp>
#include <components/vsgutil/traverse.hpp>
#include <components/vsgutil/nodepath.hpp>
#include <vsg/nodes/Switch.h>

namespace
{
    class GetTransformPath : public vsgUtil::SearchPath<const Anim::Transform*>, public vsgUtil::TConstTraverse<vsg::Node>
    {
    public:
        vsg::Node* needle{};
        using vsg::ConstVisitor::apply;
        void apply(const vsg::Transform& transform) override
        {
            accumulateCastAndTraverse(transform, *this);
        }
        void apply(const vsg::Switch& sw) override
        {
            if (&sw == needle)
                foundPath = path;
            else
                traverseNode(sw, *this);
        }
    };
}

namespace MWAnim
{
    Wielding::Wielding(const Context& ctx)
        : Actor(ctx)
    {
        for (auto& sw : mSwitches)
        {
            sw = vsg::Switch::create();
            sw->children = { { vsg::MASK_OFF, vsg::Node::create() } };
        }
    }

    Wielding::~Wielding() {}

    void Wielding::show(Wield w, bool visible)
    {
        mSwitches[static_cast<int>(w)]->setAllChildren(visible);
    }

    bool Wielding::isShown(Wield w) const
    {
        return mSwitches[static_cast<int>(w)]->children[0].mask != vsg::MASK_OFF;
    }

    std::vector<const Anim::Transform*> Wielding::getWieldWorldTransform(Wield w) const
    {
        GetTransformPath visitor;
        visitor.needle = mSwitches[static_cast<int>(w)];
        ;
        mTransform->accept(visitor);
        return visitor.foundPath;
    }

    void Wielding::addSwitch(vsg::ref_ptr<vsg::Node>& node, Wield w)
    {
        auto& sw = mSwitches[static_cast<int>(w)];
        sw->children = { { sw->children[0].mask, node } };
        node = sw;
    }

    void Wielding::update(float dt)
    {
        mWeaponControllers.update(weaponAnimationTime);
        mStaticControllers.update(0);
        Actor::update(dt);
    }
}
