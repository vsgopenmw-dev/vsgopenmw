#include "scene.hpp"

#include <vsg/io/Options.h>
#include <vsg/nodes/Switch.h>

#include <components/animation/transform.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadligh.hpp>
#include <components/mwanimation/create.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/vsgutil/removechild.hpp>
#include <components/vsgutil/compilecontext.hpp>
#include <components/vsgutil/updatethreads.hpp>
#include <components/vsgutil/operation.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "env.hpp"
#include "mask.hpp"
#include "npc.hpp"
#include "wieldingcreature.hpp"

namespace
{
    void loadTransform(const MWWorld::Ptr& ptr, Anim::Transform& trafo)
    {
        const float* f = ptr.getRefData().getPosition().pos;
        trafo.translation = { f[0], f[1], f[2] };
        const float scale = ptr.getCellRef().getScale();
        osg::Vec3f scaleVec(scale, scale, scale);
        ptr.getClass().adjustScale(ptr, scaleVec, true);
        trafo.scale = toVsg(scaleVec);
    }
}

namespace MWRender
{
    struct Scene::Cell
    {
        struct Update : public vsg::Operation
        {
            Scene::Cell& cell;
            float dt = 0;
            Update(Scene::Cell& c) : cell(c) {}
            void run() override
            {
                for (auto& [p, o] : cell.objects)
                    o->update(dt);
            }
        };
        vsg::ref_ptr<vsg::Group> group;
        Objects objects;
        vsg::ref_ptr<Update> update;
    };

    Scene::Scene(size_t updateThreads)
    {
        mActorGroup = vsg::Switch::create();
        mObjectGroup = vsg::Switch::create();
        mUpdateThreads = vsgUtil::UpdateThreads::create(updateThreads);
    }

    Scene::~Scene() {}

    vsg::ref_ptr<vsg::Node> Scene::getObjects()
    {
        return mObjectGroup;
    }

    vsg::ref_ptr<vsg::Node> Scene::getActors()
    {
        return mActorGroup;
    }

    Scene::Cell& Scene::getOrCreateCell(const MWWorld::Ptr& ptr, bool actor)
    {
        auto& map = actor ? mActorCells : mObjectCells;
        auto found = map.find(ptr.getCell());
        if (found == map.end())
        {
            auto cellnode = vsg::Group::create();
            if (actor)
                mActorGroup->addChild(Mask_Actor, cellnode);
            else
                mObjectGroup->addChild(Mask_Object, cellnode);
            auto& cell = map[ptr.getCell()];
            cell.group = cellnode;
            cell.update = new Cell::Update(cell);
            return cell;
        }
        else
            return found->second;
    }

    Scene::Cell* Scene::getCell(const MWWorld::Ptr& ptr, bool actor)
    {
        auto& map = actor ? mActorCells : mObjectCells;
        auto found = map.find(ptr.getCell());
        if (found != map.end())
            return &found->second;
        return {};
    }

    void Scene::insertModel(const MWWorld::Ptr& ptr, const std::string& mesh, bool allowLight)
    {
        const ESM::Light* light{};
        if (allowLight && ptr.getType() == ESM::Light::sRecordId)
            light = ptr.get<ESM::Light>()->mBase;
        vsg::ref_ptr<vsg::Group> decoration;
        if (auto color = getGlowColor(ptr))
            decoration = createEnv(*color);

        MWAnim::Context& ctx = allowLight ? context : noParticlesContext;

        auto anim = MWAnim::createObject(mesh, ptr.getClass().useAnim(), light, decoration, ctx);

        loadTransform(ptr, *anim->transform());

        if (!context.compileContext->compile(anim->node()))
            return;

        auto& cell = getOrCreateCell(ptr, false);
        cell.group->addChild(anim->node());
        cell.objects.emplace(ptr, std::move(anim));
    }

    void Scene::insertCreature(const MWWorld::Ptr& ptr, const std::string& mesh, bool weaponsShields)
    {
        bool biped = ptr.get<ESM::Creature>()->mBase->mFlags & ESM::Creature::Bipedal;
        std::unique_ptr<MWAnim::Object> anim;
        if (weaponsShields)
        {
            anim = MWAnim::createCreature<WieldingCreature>(mesh, biped, context);
            static_cast<WieldingCreature&>(*anim).setup(ptr);
        }
        else
            anim = MWAnim::createCreature(mesh, biped, context);
        loadTransform(ptr, *anim->transform());

        if (!context.compileContext->compile(anim->node()))
            return;

        auto& cell = getOrCreateCell(ptr, true);
        cell.group->addChild(anim->node());
        cell.objects.emplace(ptr, std::move(anim));
    }

    void Scene::insertNPC(const MWWorld::Ptr& ptr)
    {
        std::unique_ptr<MWAnim::Object> anim = std::make_unique<Npc>(context, ptr);
        loadTransform(ptr, *anim->transform());
        if (!context.compileContext->compile(anim->node()))
            return;

        auto& cell = getOrCreateCell(ptr, true);
        cell.group->addChild(anim->node());
        cell.objects.emplace(ptr, std::move(anim));
    }

    bool Scene::removeObject(const MWWorld::Ptr& ptr)
    {
        auto cell = getCell(ptr, ptr.getClass().isActor());
        if (!cell)
            return false;
        auto iter = cell->objects.find(ptr);
        if (iter != cell->objects.end())
        {
            vsgUtil::removeChild(cell->group, iter->second->node());
            cell->objects.erase(iter);
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

    void Scene::updatePtr(const MWWorld::Ptr& old, const MWWorld::Ptr& cur)
    {
        bool actor = cur.getClass().isActor();
        auto oldcell = getCell(old, actor);
        if (!oldcell)
            return;
        auto& cell = getOrCreateCell(cur, actor);
        auto iter = oldcell->objects.find(old);
        if (iter != oldcell->objects.end())
        {
            auto& anim = iter->second;
            cell.group->addChild(anim->node());
            vsgUtil::removeChild(oldcell->group, anim->node());
            cell.objects[cur] = std::move(anim);
            oldcell->objects.erase(iter);
        }
    }

    MWAnim::Object* Scene::getObject(const MWWorld::Ptr& ptr)
    {
        auto cell = getCell(ptr, ptr.getClass().isActor());
        if (!cell)
            return {};
        auto iter = cell->objects.find(ptr);
        if (iter != cell->objects.end())
            return iter->second.get();
        return {};
    }

    MWWorld::Ptr Scene::getPtr(const vsg::Node* node)
    {
        auto p = getPtr(node, mActorCells);
        if (!p.isEmpty())
            return p;
        return getPtr(node, mObjectCells);
    }

    MWWorld::Ptr Scene::getPtr(const vsg::Node* node, CellMap& map)
    {
        for (auto& [cellstore, cell] : map)
        {
            for (auto& [p, o] : cell.objects)
            {
                if (o->node() == node || o->transform() == node)
                    return p;
            }
        }
        return {};
    }

    void Scene::update(float dt)
    {
        std::vector<vsg::Operation*> operations;
        operations.reserve(mActorCells.size()+mObjectCells.size());
        auto addOperations = [&operations, dt](CellMap& map) {
            for (auto& [cellstore, cell] : map)
            {
                cell.update->dt = dt;
                operations.emplace_back(cell.update);
            }
        };
        addOperations(mActorCells);
        addOperations(mObjectCells);
        // vsgopenmw-optimization-mwrender-scene-updatethreads
        mUpdateThreads->run(operations.begin(), operations.end());
    }

    vsg::ref_ptr<vsgUtil::UpdateThreads> Scene::getUpdateThreads()
    {
        return mUpdateThreads;
    }
}
