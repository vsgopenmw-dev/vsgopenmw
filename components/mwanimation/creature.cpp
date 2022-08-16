#include "creature.hpp"

#include <components/vsgutil/addchildren.hpp>
#include <components/animation/context.hpp>
#include <components/animation/controllermap.hpp>

#include "play.hpp"
#include "clone.hpp"
#include "bones.hpp"
#include "context.hpp"

namespace MWAnim
{

Creature::Creature(const Context &mwctx, bool biped, const std::string &model)
    : Actor(mwctx)
{
    auto node = mwctx.readNode(model);
    auto result = cloneIfRequired(node, "tri bip");

    animation = std::make_unique<Play>();
    animation->bones = Bones(*node);
    assignBones();

    std::vector<std::string> files;
    if (biped)
        files.emplace_back("x" + mwctx.files.baseanim);
    files.emplace_back(model);
    animation->addAnimSources(mwctx.readAnimations(files));

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

void Creature::update(float dt)
{
    Actor::update(dt);
    autoPlay.update(dt);
    mUpdate.update(animation->time());
}

}

