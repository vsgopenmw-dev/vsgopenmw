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
#include "object.hpp"

namespace MWAnim
{
    std::pair<Anim::Animation, vsg::ref_ptr<vsg::Node>> Effect::load(const Context& in_mwctx, vsg::ref_ptr<vsg::Node> in_node, const std::string& overrideTexture, bool overrideAllTextures, const std::vector<vsg::ref_ptr<vsg::Node>>& replaceDummyNodes)
    {
        MWAnim::CloneResult result;
        if (!overrideTexture.empty() || !replaceDummyNodes.empty())
        {
            vsg::ref_ptr<vsg::Descriptor> texture;
            if (!overrideTexture.empty())
            {
                auto sampler = vsg::Sampler::create();
                vsgUtil::share_if(in_mwctx.textureOptions->sharedObjects, sampler);
                auto image = vsgUtil::readImage(overrideTexture, in_mwctx.textureOptions);
                texture = vsg::DescriptorImage::create(sampler, image);
                vsgUtil::share_if(in_mwctx.textureOptions->sharedObjects, texture);
            }
            result = cloneAndReplace(in_node, texture, overrideAllTextures, replaceDummyNodes);
        }
        else
            result = cloneIfRequired(in_node);
        return std::make_pair(result, in_node);
    }

    void Effect::compile(Anim::Animation& animation, vsg::ref_ptr<vsg::Node> in_node, const std::vector<Anim::Transform*>& worldAttachmentPath, const std::vector<vsg::Node*>& localAttachmentPath)
    {
        node = in_node;

        auto ctx = Anim::Context{ worldAttachmentPath, localAttachmentPath, {}, &mwctx.mask };
        animation.link(ctx, [this](const Anim::Controller* ctrl, vsg::Object* o) {
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
        parentBone = p;
        parentBone->addChild(node);
    }

    void Effect::attachTo(MWAnim::Object* obj)
    {
        // assert(compiled);
        parentObject = obj;
        obj->nodeToAddChildrenTo()->addChild(node);
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
        if (parentBone)
        {
            vsgUtil::removeChild(parentBone, node);
            parentBone = {};
        }
        else if (parentObject)
        {
            vsgUtil::removeChild(parentObject->nodeToAddChildrenTo(), node);
            parentObject = {};
        }

        mwctx.compileContext->detach(node);
    }
}
