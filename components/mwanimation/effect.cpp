#include "effect.hpp"

#include <vsg/state/DescriptorImage.h>

#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/removechild.hpp>
#include <components/animation/context.hpp>
#include <components/animation/update.hpp>

#include "clone.hpp"
#include "context.hpp"

namespace MWAnim
{
    void Effect::compile(const MWAnim::Context &mwctx)
    {
        MWAnim::CloneResult result;
        if (!overrideTexture.empty() || !replaceDummyNodes.empty())
        {
            auto texture = vsgUtil::readDescriptorImage(overrideTexture, mwctx.textureOptions);
            result = cloneAndReplace(node, texture, overrideAllTextures, replaceDummyNodes);
        }
        else
            result = cloneIfRequired(node);

        auto ctx = Anim::Context{node.get(), {}, &mwctx.mask};
        result.link(ctx);
        result.accept([this](const Anim::Controller *ctrl, vsg::Object *o) {
            update.controllers.emplace_back(ctrl, o);
            duration = std::max(ctrl->hints.duration, duration);
        });
        if (!mwctx.compile(node))
        {
            update = {};
            node = vsg::Node::create();
        }
    }

    void Effect::attachTo(vsg::Group *p)
    {
        //assert(compiled);
        parent = p;
        parent->addChild(node);
    }

    bool Effect::run(float dt)
    {
        update.update(dt);
        if (update.timer >= duration)
        {
            if (loop)
            {
                // Start from the beginning again; carry over the remainder
                // Not sure if this is actually needed, the controller function might already handle loops
                update.timer -= duration;
            }
            else
                return false;
        }
        return true;
    }

    void Effect::detach()
    {
        vsgUtil::removeChild(parent, node);
    }
}
