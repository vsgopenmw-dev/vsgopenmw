#include "effects.hpp"

#include <components/animation/transform.hpp>
#include <components/mwanimation/context.hpp>
#include <components/mwanimation/effect.hpp>
#include <components/mwanimation/effects.hpp>

#include "mask.hpp"

namespace MWRender
{
    Effects::Effects(const MWAnim::Context& ctx)
        : mContext(ctx)
    {
        mContext.compileContext = mContext.compileContext->clone(Mask_Effect);

        mNode = vsg::Group::create();
        mEffects = new MWAnim::Effects;
    }

    Effects::~Effects() {}

    void Effects::add(const std::string& model, const std::string& textureOverride, const vsg::vec3& worldPosition,
        float scale, bool isMagicVFX)
    {
        auto transform = vsg::ref_ptr{ new Anim::Transform };
        transform->translation = worldPosition;
        transform->setScale(scale);

        MWAnim::Effect effect;
        effect.mwctx = mContext;

        auto [anim, node] = MWAnim::Effect::load(mContext, mContext.readNode(model), textureOverride, !isMagicVFX);
        transform->children = { node };
        effect.compile(anim, transform, { transform }, { node.get() } );

        effect.attachTo(mNode.get());
        mEffects->effects.push_back(effect);
    }

    void Effects::update(float dt)
    {
        mEffects->update(dt);
    }

    void Effects::clear()
    {
        mEffects->remove();
    }
}
