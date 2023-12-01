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
        MWAnim::Effect effect{.mwctx=mContext};
        effect.overrideTexture = textureOverride;
        effect.overrideAllTextures = !isMagicVFX;
        effect.node = mContext.readNode(model);
        effect.compile();

        auto transform = vsg::ref_ptr{ new Anim::Transform };
        transform->children = { effect.node };
        effect.node = transform;

        effect.attachTo(mNode.get());

        transform->translation = worldPosition;
        transform->setScale(scale);

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
