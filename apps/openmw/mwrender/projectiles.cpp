#include "projectiles.hpp"

#include <vsg/nodes/Light.h>

#include <components/animation/constant.hpp>
#include <components/animation/meta.hpp>
#include <components/animation/roll.hpp>
#include <components/animation/transform.hpp>
#include <components/mwanimation/context.hpp>
#include <components/mwanimation/effect.hpp>

#include "env.hpp"

namespace MWRender
{
    struct Projectile
    {
        std::shared_ptr<ProjectileHandle> handle;
        vsg::ref_ptr<Anim::Transform> transform;
        MWAnim::Effect effect;
    };

    Projectiles::Projectiles(const MWAnim::Context& c)
        : mContext(c)
    {
        mNode = vsg::Group::create();
    }

    Projectiles::~Projectiles() {}

    std::shared_ptr<ProjectileHandle> Projectiles::add(const std::string& model, const vsg::vec3& pos,
        const vsg::quat& orient, bool autoRotate, std::optional<vsg::vec3> lightColor,
        std::optional<vsg::vec4> glowColor, const std::string textureOverride,
        const std::vector<std::string> additionalModels)
    {
        std::vector<vsg::ref_ptr<vsg::Node>> replaceDummyNodes;
        for (auto& m : additionalModels)
            replaceDummyNodes.push_back(mContext.readNode(m));

        auto [anim, node] = MWAnim::Effect::load(mContext, mContext.readNode(model), textureOverride, true, replaceDummyNodes);

        Projectile p;
        p.effect.mwctx = mContext;
        p.handle = std::make_shared<ProjectileHandle>(ProjectileHandle{ pos, orient });
        p.transform = Anim::Transform::create();
        p.transform->translation = pos;
        p.transform->setAttitude(orient);

        addEnv(node, glowColor);

        std::vector<Anim::Transform*> worldAttachmentPath = { p.transform.get() };
        std::vector<vsg::Node*> localAttachmentPath = { node.get() };
        if (autoRotate)
        {
            auto rotateNode = Anim::Transform::create();
            auto ctrl = Anim::Roll::create();
            ctrl->axis = { 0, -1, 0 };
            ctrl->speed = Anim::make_constant(vsg::PIf * 2);
            ctrl->attachTo(*rotateNode);
            p.effect.update.add(ctrl, rotateNode);
            p.transform->children = { rotateNode };
            rotateNode->children = { node };
            worldAttachmentPath.push_back(rotateNode.get());
        }
        else
            p.transform->children = { node };

        if (lightColor)
        {
            auto light = vsg::PointLight::create();
            light->color = *lightColor;
            // light->ambientColor = {1,1,1};
            light->intensity = 30;
            p.transform->addChild(light);
        }

        p.effect.compile(anim, p.transform, worldAttachmentPath, localAttachmentPath);

        p.effect.attachTo(mNode);
        mProjectiles.push_back(p);
        return p.handle;
    }

    void Projectiles::update(float dt)
    {
        for (auto it = mProjectiles.begin(); it != mProjectiles.end();)
        {
            auto& p = *it;
            if (p.handle->remove)
            {
                p.effect.detach();
                it = mProjectiles.erase(it);
            }
            else
            {
                p.transform->translation = p.handle->position;
                p.transform->setAttitude(p.handle->attitude);
                p.effect.run(dt);
                ++it;
            }
        }
    }
}
