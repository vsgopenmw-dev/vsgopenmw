#include "effect.hpp"

#include <vsg/state/DescriptorImage.h>
#include <vsg/utils/SharedObjects.h>

#include <components/animation/context.hpp>
#include <components/animation/update.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/removechild.hpp>
#include <components/vsgutil/compilecontext.hpp>
#include <components/vsgutil/share.hpp>

#include "clone.hpp"
#include "context.hpp"

namespace MWAnim
{
    void Effect::compile()
    {
        MWAnim::CloneResult result;
        if (!overrideTexture.empty() || !replaceDummyNodes.empty())
        {
            vsg::ref_ptr<vsg::Descriptor> texture;
            if (!overrideTexture.empty())
            {
                auto sampler = vsg::Sampler::create();
                vsgUtil::share_if(mwctx.textureOptions->sharedObjects, sampler);
                auto image = vsgUtil::readImage(overrideTexture, mwctx.textureOptions);
                texture = vsg::DescriptorImage::create(sampler, image);
                vsgUtil::share_if(mwctx.textureOptions->sharedObjects, texture);
            }
            result = cloneAndReplace(node, texture, overrideAllTextures, replaceDummyNodes);
        }
        else
            result = cloneIfRequired(node);

        auto ctx = Anim::Context{ {node.get()}, {}, &mwctx.mask };
        result.link(ctx, [this](const Anim::Controller* ctrl, vsg::Object* o) {
            update.add(ctrl, o);
            duration = std::max(ctrl->hints.duration, duration);
        });
        if (!mwctx.compileContext->compile(node))
        {
            update = {};
            node = vsg::Node::create();
        }
    }

    void Effect::attachTo(vsg::Group* p)
    {
        // assert(compiled);
        parent = p;
        parent->addChild(node);
    }

    bool Effect::run(float dt)
    {
        update.update(dt);
        auto& group = update.getOrCreateGroup();
        if (group.timer >= duration)
        {
            if (loop)
            {
                // Start from the beginning again; carry over the remainder
                // Not sure if this is actually needed, the controller function might already handle loops
                group.timer -= duration;
            }
            else
                return false;
        }
        return true;
    }

    void Effect::detach()
    {
        if (parent)
        {
            vsgUtil::removeChild(parent, node);
            mwctx.compileContext->detach(node);
        }
    }
}
