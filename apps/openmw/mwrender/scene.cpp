#include "scene.hpp"

#include <vsg/io/Options.h>
#include <vsg/nodes/Switch.h>

#include <components/esm3/loadligh.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/vsgutil/removechild.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/animation/transform.hpp>
#include <components/mwanimation/creature.hpp>
#include <components/mwanimation/objectanim.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"

#include "npc.hpp"
#include "wieldingcreature.hpp"
#include "animcontext.hpp"
#include "mask.hpp"

namespace
{
    void loadTransform(const MWWorld::Ptr &ptr, Anim::Transform &trafo)
    {
        const float *f = ptr.getRefData().getPosition().pos;
        trafo.translation = {f[0], f[1], f[2]};
        const float scale = ptr.getCellRef().getScale();
        osg::Vec3f scaleVec(scale, scale, scale);
        ptr.getClass().adjustScale(ptr, scaleVec, true);
        trafo.scale = toVsg(scaleVec);
    }
}

namespace MWRender
{

Scene::Scene(Resource::ResourceSystem* resourceSystem, vsg::CompileManager *compile)
{
    context = animContext(resourceSystem, compile);
    noParticlesContext = context;
    noParticlesContext.mask.particle = vsg::MASK_OFF;

    mActorGroup = vsg::Switch::create();
    mObjectGroup = vsg::Switch::create();
}

Scene::~Scene()
{
}

vsg::ref_ptr<vsg::Node> Scene::getObjects()
{
    return mObjectGroup;
}

vsg::ref_ptr<vsg::Node> Scene::getActors()
{
    return mActorGroup;
}

Scene::Cell &Scene::getOrCreateCell(const MWWorld::Ptr& ptr, bool actor)
{
    auto &map = actor ? mActorCells : mObjectCells;
    auto found = map.find(ptr.getCell());
    if (found == map.end())
    {
        auto cellnode = vsg::Group::create();
        if (actor)
            mActorGroup->addChild(Mask_Actor, cellnode);
        else
            mObjectGroup->addChild(Mask_Object, cellnode);
        auto cell = Cell{cellnode, Objects()};
        auto inserted = map.emplace(ptr.getCell(), std::move(cell));
        return inserted.first->second;
    }
    else
        return found->second;
}

Scene::Cell *Scene::getCell(const MWWorld::Ptr& ptr, bool actor)
{
    auto &map = actor ? mActorCells : mObjectCells;
    auto found = map.find(ptr.getCell());
    if (found != map.end())
        return &found->second;
    return {};
}

void Scene::insertModel(const MWWorld::Ptr &ptr, const std::string &mesh, bool animated, bool allowLight)
{
    std::unique_ptr<MWAnim::Object> anim;
    const ESM::Light *light{};
    if (allowLight && ptr.getType() == ESM::Light::sRecordId)
        light = ptr.get<ESM::Light>()->mBase;
    MWAnim::Context &ctx = allowLight ? context : noParticlesContext;
    if (animated)
    {
        if (mesh.empty())
        {
            if (!light)
                return;
            anim = std::make_unique<MWAnim::Auto>(ctx, *light);
        }
        else
            anim = std::make_unique<MWAnim::ObjectAnimation>(ctx, mesh, light);
    }
    else
        anim = std::make_unique<MWAnim::Auto>(ctx, mesh, light);
    loadTransform(ptr, *anim->transform());

    if (compile && !context.compile(anim->node()))
        return;

    auto &cell = getOrCreateCell(ptr, false);
    cell.group->addChild(anim->node());
    cell.objects.emplace(ptr, std::move(anim));
}

void Scene::insertCreature(const MWWorld::Ptr &ptr, const std::string &mesh, bool weaponsShields)
{
    bool biped = ptr.get<ESM::Creature>()->mBase->mFlags & ESM::Creature::Bipedal;
    std::unique_ptr<MWAnim::Object> anim;
    if (weaponsShields)
        anim = std::make_unique<WieldingCreature>(context, ptr, biped, mesh);
    else
        anim = std::make_unique<MWAnim::Creature>(context, biped, mesh);
    loadTransform(ptr,*anim->transform());

    if (compile && !context.compile(anim->node()))
        return;

    auto &cell = getOrCreateCell(ptr, true);
    cell.group->addChild(anim->node());
    cell.objects.emplace(ptr, std::move(anim));
}

void Scene::insertNPC(const MWWorld::Ptr &ptr)
{
    std::unique_ptr<MWAnim::Object> anim = std::make_unique<Npc>(context, ptr);
    loadTransform(ptr,*anim->transform());
    if (compile && !context.compile(anim->node()))
        return;

    auto &cell = getOrCreateCell(ptr, true);
    cell.group->addChild(anim->node());
    cell.objects.emplace(ptr, std::move(anim));
}

bool Scene::removeObject (const MWWorld::Ptr& ptr)
{
    auto cell = getCell(ptr, ptr.getClass().isActor());
    if (!cell)
        return false;
    auto iter = cell->objects.find(ptr);
    if(iter != cell->objects.end())
    {
        vsgUtil::removeChild(cell->group, iter->second->node());
        cell->objects.erase(iter);
    //deletionQueue.add(
        return true;
    }
    return false;
}


void Scene::removeCell(const MWWorld::CellStore* store)
{
    auto iter = mActorCells.find(store);
    if (iter != mActorCells.end())
    {
        vsgUtil::removeSwitchChild(mActorGroup, iter->second.group);
        mActorCells.erase(iter);
    }
    auto iter2 = mObjectCells.find(store);
    if (iter2 != mObjectCells.end())
    {
        vsgUtil::removeSwitchChild(mObjectGroup, iter2->second.group);
        mObjectCells.erase(iter2);
    }
}

void Scene::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &cur)
{
    bool actor = cur.getClass().isActor();
    auto oldcell = getCell(old, actor);
    if (oldcell)
        return;
    auto &cell = getOrCreateCell(cur, actor);
    auto iter = oldcell->objects.find(old);
    if(iter != oldcell->objects.end())
    {
        auto &anim = iter->second;
        cell.group->addChild(anim->node());
        vsgUtil::removeChild(oldcell->group, anim->node());
        cell.objects[cur] = std::move(anim);
        oldcell->objects.erase(iter);
    }
}

MWAnim::Object* Scene::getObject(const MWWorld::Ptr &ptr)
{
    auto cell = getCell(ptr, ptr.getClass().isActor());
    if (!cell)
        return {};
    auto iter = cell->objects.find(ptr);
    if(iter != cell->objects.end())
        return iter->second.get();
    return {};
}

MWWorld::Ptr Scene::getPtr(const vsg::Node *node)
{
    if (auto p = getPtr(node, mActorCells))
        return p;
    return getPtr(node, mObjectCells);
}

MWWorld::Ptr Scene::getPtr(const vsg::Node *node, CellMap &map)
{
    for (auto &[cellstore, cell] : map)
    {
        for (auto &[p, o] : cell.objects)
        {
            if (o->node() == node || o->transform() == node)
                return p;
        }
    }
    return {};
}

void Scene::update(float dt)
{
    update(dt, mObjectCells);
    update(dt, mActorCells);
}

void Scene::update(float dt, CellMap &map)
{
    for (auto &[cellstore, cell] : map)
        for (auto &[p, o] : cell.objects)
            o->update(dt);
}

}
