#include "projectiles.hpp"

#include <vsg/nodes/Light.h>

#include <components/animation/roll.hpp>
#include <components/animation/constant.hpp>
#include <components/animation/transform.hpp>
#include <components/animation/meta.hpp>
#include <components/mwanimation/effect.hpp>
#include <components/mwanimation/context.hpp>

#include "env.hpp"

namespace MWRender
{
    struct Projectile
    {
        std::shared_ptr<ProjectileHandle> handle;
        vsg::ref_ptr<Anim::Transform> transform;
        MWAnim::Effect effect;
    };

    Projectiles::Projectiles(const MWAnim::Context &c)
        : mContext(c)
    {
        mNode = vsg::Group::create();
    }

    Projectiles::~Projectiles()
    {
    }

    std::shared_ptr<ProjectileHandle> Projectiles::add(const std::string &model, const vsg::vec3 &pos, const vsg::quat &orient, bool autoRotate, std::optional<vsg::vec3> lightColor, std::optional<vsg::vec4> glowColor, const std::string textureOverride, const std::vector<std::string> additionalModels)
    {
        Projectile p;
        p.handle = std::make_shared<ProjectileHandle>(ProjectileHandle{pos, orient});
        p.transform = new Anim::Transform;
        p.transform->translation = pos;
        p.transform->setAttitude(orient);

        p.effect = {.node=mContext.readNode(model)};
        auto meta = Anim::Meta::get(*p.effect.node);
        addEnv(p.effect.node, glowColor);
        if (meta)
            meta->attachTo(*p.effect.node);

        p.effect.overrideTexture = textureOverride;
        for (auto &m : additionalModels)
            p.effect.replaceDummyNodes.push_back(mContext.readNode(m));
        p.effect.compile(mContext);

        if (autoRotate)
        {
            auto rotateNode = vsg::ref_ptr{new Anim::Transform};
            auto ctrl = vsg::ref_ptr{new Anim::Roll};
            ctrl->axis = {0,-1,0};
            ctrl->speed = new Anim::Constant(vsg::PIf * 2);
            ctrl->attachTo(*rotateNode);
            p.effect.update.controllers.emplace_back(ctrl, rotateNode);
            p.transform->children = {rotateNode};
            rotateNode->children = {p.effect.node};
        }
        else
            p.transform->children = {p.effect.node};

        if (lightColor)
        {
            auto light = vsg::PointLight::create();
            light->color = *lightColor;
            //light->ambientColor = {1,1,1};
            light->intensity = 30;
            p.transform->addChild(light);
        }

        p.effect.node = p.transform;
        p.effect.attachTo(mNode);
        mProjectiles.push_back(p);
        return p.handle;
    }

    void Projectiles::update(float dt)
    {
        for (auto it = mProjectiles.begin(); it != mProjectiles.end(); )
        {
            auto &p = *it;
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
