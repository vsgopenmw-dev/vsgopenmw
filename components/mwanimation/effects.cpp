#include "effects.hpp"

#include "effect.hpp"
#include "object.hpp"

namespace MWAnim
{
    const std::string Effects::sAttachKey = "fx";

    void Effects::remove_if(std::function<bool(Effect&)> f)
    {
        for (auto it = effects.begin(); it != effects.end();)
        {
            if (f(*it))
            {
                it->detach();
                it = effects.erase(it);
            }
            else
                ++it;
        }
    }

    void Effects::update(float dt)
    {
        remove_if([dt](Effect& e) { return !e.run(dt); });
    }

    void Effects::remove(std::optional<int> effectId)
    {
        remove_if([effectId](Effect& e) { return !effectId || e.effectId == *effectId; });
    }

    void updateEffects(vsg::Node& root, float dt)
    {
        if (auto effects = Effects::get(root))
            effects->update(dt);
    }

    void addEffect(MWAnim::Object& obj, vsg::Group* attachBone, vsg::ref_ptr<vsg::Node> prototype, const std::vector<Anim::Transform*>& worldAttachmentPath,
        int effectId, bool loop, const std::string& overrideTexture, bool overrideAllTextures)
    {
        auto effects = Effects::getOrCreate(*obj.node());
        for (auto& e : effects->effects)
        {
            if (loop && e.loop && e.parentBone == attachBone)
                return;
        }

        // node->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        Effect effect;
        effect.mwctx = obj.context();
        effect.effectId = effectId;
        effect.loop = loop;

        auto [anim, node] = Effect::load(obj.context(), prototype, overrideTexture, overrideAllTextures);
        effect.compile(anim, node, worldAttachmentPath, { node });

        if (attachBone)
            effect.attachTo(attachBone);
        else
            effect.attachTo(&obj);

        effects->effects.push_back(effect);
    }

    void removeEffect(vsg::Node& root, std::optional<int> effectId)
    {
        if (auto effects = Effects::get(root))
            effects->remove(effectId);
        if (!effectId)
            root.removeObject(Effects::sAttachKey);
    }

    void getLoopingEffects(const vsg::Node& root, std::vector<int>& out)
    {
        if (auto effects = Effects::get(root))
            for (auto& e : effects->effects)
                if (e.loop)
                    out.push_back(e.effectId);
    }
}
