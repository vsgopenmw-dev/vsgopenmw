#include "effect.hpp"

#include <vsg/nodes/Switch.h>

#include <components/mwanimation/object.hpp>
#include <components/mwanimation/context.hpp>
#include <components/mwanimation/effects.hpp>
#include <components/mwanimation/play.hpp>
#include <components/animation/transform.hpp>
#include <components/animation/attachbone.hpp>
#include <components/animation/meta.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "mask.hpp"

namespace MWRender
{
    void addEffect(const MWWorld::Ptr &ptr, const std::string &effectRefId, int magicEffectId, bool loop, const std::string &bone, const std::string &overrideTexture)
    {
        auto world = MWBase::Environment::get().getWorld();
        auto anim = world->getAnimation(ptr);
        if (!anim || !anim->animation) // if (!hasCharacterController)
            return;
        auto &ctx = anim->context();

        auto static_ = world->getStore().get<ESM::Static>().search(effectRefId);
        if (!static_ || static_->mModel.empty())
            return;
        auto node = ctx.readNode(static_->mModel);
        auto meta = Anim::Meta::get(*node);

        vsg::Group *attachTo = anim->transform();
        if (!bone.empty())
        {
            auto b = anim->searchBone(bone);
            if (!b)
                return;
            attachTo = Anim::getOrAttachBone(attachTo, b->path);
        }
        else if (!ptr.getClass().isNpc())
        {
            auto trans = vsg::ref_ptr{new Anim::Transform};
            osg::Vec3f bounds (MWBase::Environment::get().getWorld()->getHalfExtents(ptr) * 2.f);
            float scale = std::max({bounds.x(), bounds.y(), bounds.z() / 2.f}) / 64.f;
            if (scale > 1.f)
                trans->setScale(scale);
            float offset = 0.f;
            if (bounds.z() < 128.f)
                offset = bounds.z() - 128.f;
            else if (bounds.z() < bounds.x() + bounds.y())
                offset = 128.f - bounds.z();
            if (world->isFlying(ptr))
                offset /= 20.f;
            trans->translation = {0.f, 0.f, offset * scale};
            trans->children = {node};
                meta->attachTo(*trans);
            node = trans;
        }

        auto sw = vsg::Switch::create();
        sw->children = {{Mask_Effect, node}};
        node = sw;
        if (meta)
            meta->attachTo(*node);

        MWAnim::addEffect(ctx, *anim->transform(), *attachTo, node, magicEffectId, loop, overrideTexture);
    }
    void removeEffect(const MWWorld::Ptr &ptr, std::optional<int> effectId)
    {
        if (auto anim = MWBase::Environment::get().getWorld()->getAnimation(ptr))
            MWAnim::removeEffect(*anim->transform(), effectId);
    }
}
