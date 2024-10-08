#ifndef VSGOPENMW_MWANIMATION_CREATE_H
#define VSGOPENMW_MWANIMATION_CREATE_H

#include <components/vsgutil/addchildren.hpp>
#include <components/animation/controllermap.hpp>
#include <components/animation/context.hpp>

#include "clone.hpp"
#include "play.hpp"
#include "bones.hpp"
#include "light.hpp"
#include "objectanim.hpp"
#include "actor.hpp"

namespace MWAnim
{
    /*
     * Assembles animation scene graph.
     */
    template <class ObjectType>
    void addNode(ObjectType& obj, Anim::Animation& controllers, vsg::ref_ptr<vsg::Node> node)
    {
        Anim::Bones* bones = obj.animation ? &obj.animation->bones : nullptr;
        Anim::Context ctx{ { obj.transform() }, { node.get() }, bones, &obj.context().mask };

        controllers.link(ctx, [&obj](const Anim::Controller* ctrl, vsg::Object* target) {
            obj.addController(ctrl, target);
        });

        vsgUtil::addChildren(*obj.nodeToAddChildrenTo(), *node);
    }

    template <class ObjectType>
    std::unique_ptr<ObjectType> create(const Context& ctx, vsg::ref_ptr<vsg::Node>& node, const ESM::Light* light, bool bones)
    {
        auto obj = std::make_unique<ObjectType>(ctx);
        CloneResult result;
        if (light)
        {
            auto presult = clonePlaceholdersIfRequired(node);
            addLight(presult.placeholders.attachLight, obj->transform(), *light, obj->autoPlay.getOrCreateGroup().controllers);
            result = presult;
        }
        else
            result = cloneIfRequired(node);

        if (bones)
        {
            obj->animation = std::make_unique<Play>(Bones(*node));
            obj->assignBones();
        }

        addNode(*obj, result, node);
        return obj;
    }

    std::unique_ptr<Object> createObject(const std::string& mesh, bool useAnim, const ESM::Light* light, const Context& ctx)
    {
        std::unique_ptr<Object> obj;
        if (mesh.empty())
        {
            if (!light)
                return {};
            obj = std::make_unique<Object>(ctx);
            addLight({}, obj->transform(), *light, obj->autoPlay.getOrCreateGroup().controllers);
            return obj;
        }
        else
        {
            vsg::ref_ptr<vsg::Node> node;
            if (useAnim)
            {
                node = ctx.readActor(mesh);
                obj = create<ObjectAnimation>(ctx, node, light, true);
                if (auto anim = ctx.readAnimation(mesh))
                    obj->animation->addSingleAnimSource(anim, mesh);
            }
            else
            {
                node = ctx.readNode(mesh);
                obj = create<Object>(ctx, node, light, false);
            }
            return obj;
        }
    }

    template <class Creature=Actor>
    std::unique_ptr<Creature> createCreature(const std::string& mesh, bool biped, const Context& ctx)
    {
        auto node = ctx.readActor(mesh);
        auto result = cloneIfRequired(node, "tri bip");
        auto obj = std::make_unique<Creature>(ctx);
        obj->animation = std::make_unique<Play>(Bones(*node));
        obj->assignBones();
        std::vector<std::string> files;
        if (biped)
            files.emplace_back(ctx.files.baseanim);
        files.emplace_back(mesh);
        obj->animation->addAnimSources(ctx.readAnimations(files), mesh);
        addNode(*obj, result, node);
        return obj;
    }
}

#endif

