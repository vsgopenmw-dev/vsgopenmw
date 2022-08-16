#include "auto.hpp"

#include <components/vsgutil/addchildren.hpp>
#include <components/animation/context.hpp>
#include <components/animation/transform.hpp>

#include "clone.hpp"
#include "light.hpp"
#include "context.hpp"

namespace MWAnim
{
    Auto::Auto(const Context &ctx)
        : Object(ctx)
    {
    }

    Auto::Auto(const Context &ctx, const ESM::Light &light)
        : Object(ctx)
    {
        addLight({}, mTransform.get(), light, autoPlay.controllers);
    }

    Auto::Auto(const Context &mwctx, const std::string &model, const ESM::Light *light)
        : Object(mwctx)
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

        auto ctx = Anim::Context{node.get(), {}, &mwctx.mask};
        result.link(ctx);
        result.accept([this](const Anim::Controller *ctrl, vsg::Object *o) { if (ctrl->hints.autoPlay) autoPlay.controllers.emplace_back(ctrl, o); });

        //mTransform = findNonControlledTransform(node)
        vsgUtil::addChildren(*mTransform, *node);
        /*
        if (SceneUtil::hasUserDescription(mObjectRoot, Constants::NightDayLabel))
        {
            AddSwitchCallbacksVisitor visitor;
            mObjectRoot->accept(visitor);
        }

        //   mCanBeHarvested = ptr.getType() == ESM::Container::sRecordId && (ptr.get<ESM::Container>()->mBase->mFlags & ESM::Container::Organic)  && SceneUtil::hasUserDescription(mObjectRoot, Constants::HerbalismLabel)       ;
        if (ptr.getRefData().getCustomData() != nullptr && canBeHarvested())
        {
            const MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
            if (!store.hasVisibleItems())
            {
                HarvestVisitor visitor;
                mObjectRoot->accept(visitor);
            }
        }
        */
    }

    void Auto::update(float dt)
    {
        autoPlay.update(dt);
    }
}
