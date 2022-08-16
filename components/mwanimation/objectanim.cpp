#include "objectanim.hpp"

#include <components/vsgutil/addchildren.hpp>
#include <components/animation/context.hpp>
#include <components/animation/controllermap.hpp>

#include "play.hpp"
#include "clone.hpp"
#include "bones.hpp"
#include "light.hpp"
#include "context.hpp"

namespace MWAnim
{
    ObjectAnimation::ObjectAnimation(const Context &mwctx, const std::string &model, const ESM::Light *light)
        : Auto(mwctx)
    {
        auto node = mwctx.readNode(model);
        MWAnim::CloneResult result;
        if (light)
        {
            auto presult = clonePlaceholdersIfRequired(node);
            addLight(presult.placeholders.attachLight, mTransform.get(), *light, autoPlay.controllers);
            result = presult;
        }
        else
            result = cloneIfRequired(node);

        animation = std::make_unique<Play>();
        animation->bones = Bones(*node);
        if (auto anim = mwctx.readAnimation(model))
            animation->addSingleAnimSource(anim);

        Anim::Context ctx{node.get(), &animation->bones, &mwctx.mask};
        result.link(ctx);
        result.accept([this](const Anim::Controller *ctrl, vsg::Object *o) {
            if (ctrl->hints.autoPlay)
                autoPlay.controllers.emplace_back(ctrl, o);
            else
                mUpdate.controllers.emplace_back(ctrl, o);
        });
        vsgUtil::addChildren(*mTransform, *node);
    }

    void ObjectAnimation::update(float dt)
    {
        Auto::update(dt);
        mUpdate.update(animation->time());
    }
}
