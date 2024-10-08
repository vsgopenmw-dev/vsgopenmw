#include "scene.hpp"

#include <atomic>
#include <chrono>
#include <limits>

#include <vsg/io/Options.h>
#include <vsg/threading/OperationThreads.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include <components/vsgadapters/osgcompat.hpp>
#include <components/terrain/preload.hpp>
#include <components/debug/debuglog.hpp>
#include <components/detournavigator/agentbounds.hpp>
#include <components/detournavigator/debug.hpp>
#include <components/detournavigator/heightfieldshape.hpp>
#include <components/detournavigator/navigator.hpp>
#include <components/detournavigator/updateguard.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm/records.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/misc/convert.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/values.hpp>

#include "../mwstate/loading.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/landmanager.hpp"
#include "../mwrender/rendermanager.hpp"

#include "../mwphysics/actor.hpp"
#include "../mwphysics/heightfield.hpp"
#include "../mwphysics/object.hpp"
#include "../mwphysics/physicssystem.hpp"

#include "cellstore.hpp"
#include "cellvisitors.hpp"
#include "class.hpp"
#include "esmstore.hpp"
#include "localscripts.hpp"
#include "player.hpp"
#include "worldimp.hpp"
#include "preloadcell.hpp"

namespace
{
    using MWWorld::RotationOrder;

    osg::Quat makeActorOsgQuat(const ESM::Position& position)
    {
        return osg::Quat(position.rot[2], osg::Vec3(0, 0, -1));
    }

    osg::Quat makeInversedOrderObjectOsgQuat(const ESM::Position& position)
    {
        const float xr = position.rot[0];
        const float yr = position.rot[1];
        const float zr = position.rot[2];

        return osg::Quat(xr, osg::Vec3(-1, 0, 0)) * osg::Quat(yr, osg::Vec3(0, -1, 0))
            * osg::Quat(zr, osg::Vec3(0, 0, -1));
    }

    osg::Quat makeInverseNodeRotation(const MWWorld::Ptr& ptr)
    {
        const auto pos = ptr.getRefData().getPosition();
        return ptr.getClass().isActor() ? makeActorOsgQuat(pos) : makeInversedOrderObjectOsgQuat(pos);
    }

    osg::Quat makeDirectNodeRotation(const MWWorld::Ptr& ptr)
    {
        const auto pos = ptr.getRefData().getPosition();
        return ptr.getClass().isActor() ? makeActorOsgQuat(pos) : Misc::Convert::makeOsgQuat(pos);
    }

    osg::Quat makeNodeRotation(const MWWorld::Ptr& ptr, RotationOrder order)
    {
        if (order == RotationOrder::inverse)
            return makeInverseNodeRotation(ptr);
        return makeDirectNodeRotation(ptr);
    }
    void setNodeRotation(const MWWorld::Ptr& ptr, MWRender::RenderManager& rendering, const osg::Quat& rotation)
    {
        rendering.rotateObject(ptr, rotation);
    }

    std::string getModel(const MWWorld::Ptr& ptr)
    {
        if (Misc::ResourceHelpers::isHiddenMarker(ptr.getCellRef().getRefId()))
            return {};
        return ptr.getClass().getModel(ptr);
    }

    void addObject(const MWWorld::Ptr& ptr, const MWWorld::World& world, const std::vector<ESM::RefNum>& pagedRefs,
        MWPhysics::PhysicsSystem& physics, MWRender::RenderManager& rendering)
    {
        if (physics.getActor(ptr))
        {
            Log(Debug::Warning) << "Warning: Tried to add " << ptr.getCellRef().getRefId() << " to the scene twice";
            return;
        }

        std::string model = getModel(ptr);
        const auto rotation = makeDirectNodeRotation(ptr);

        ESM::RefNum refnum = ptr.getCellRef().getRefNum();
        if (!refnum.hasContentFile() || !std::binary_search(pagedRefs.begin(), pagedRefs.end(), refnum))
            ptr.getClass().insertObjectRendering(ptr, model, rendering.getObjects());
        setNodeRotation(ptr, rendering, rotation);

        if (ptr.getClass().useAnim())
            MWBase::Environment::get().getMechanicsManager()->add(ptr);

        if (ptr.getClass().isActor())
            rendering.addWaterRippleEmitter(ptr);

        // Restore effect particles
        world.applyLoopingParticles(ptr);

        if (!model.empty())
            ptr.getClass().insertObject(ptr, model, rotation, physics);

        MWBase::Environment::get().getLuaManager()->objectAddedToScene(ptr);
    }

    void addObject(const MWWorld::Ptr& ptr, const MWWorld::World& world, const MWPhysics::PhysicsSystem& physics,
        DetourNavigator::Navigator& navigator, const DetourNavigator::UpdateGuard* navigatorUpdateGuard = nullptr)
    {
        if (const auto object = physics.getObject(ptr))
        {
            const DetourNavigator::ObjectTransform objectTransform{ ptr.getRefData().getPosition(),
                ptr.getCellRef().getScale() };

            if (ptr.getClass().isDoor() && !ptr.getCellRef().getTeleport())
            {
                btVector3 aabbMin;
                btVector3 aabbMax;
                object->getShapeInstance()->mCollisionShape->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

                const auto center = (aabbMax + aabbMin) * 0.5f;

                const auto distanceFromDoor = world.getMaxActivationDistance() * 0.5f;
                const auto toPoint = aabbMax.x() - aabbMin.x() < aabbMax.y() - aabbMin.y()
                    ? btVector3(distanceFromDoor, 0, 0)
                    : btVector3(0, distanceFromDoor, 0);

                const auto transform = object->getTransform();
                const btTransform closedDoorTransform(
                    Misc::Convert::makeBulletQuaternion(ptr.getCellRef().getPosition()), transform.getOrigin());

                const auto start = Misc::Convert::toOsg(closedDoorTransform(center + toPoint));
                const auto startPoint = physics.castRay(start, start - osg::Vec3f(0, 0, 1000), ptr, {},
                    MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap
                        | MWPhysics::CollisionType_Water);
                const auto connectionStart = startPoint.mHit ? startPoint.mHitPos : start;

                const auto end = Misc::Convert::toOsg(closedDoorTransform(center - toPoint));
                const auto endPoint = physics.castRay(end, end - osg::Vec3f(0, 0, 1000), ptr, {},
                    MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap
                        | MWPhysics::CollisionType_Water);
                const auto connectionEnd = endPoint.mHit ? endPoint.mHitPos : end;

                navigator.addObject(DetourNavigator::ObjectId(object),
                    DetourNavigator::DoorShapes(
                        object->getShapeInstance(), objectTransform, connectionStart, connectionEnd),
                    transform, navigatorUpdateGuard);
            }
            else if (object->getShapeInstance()->mVisualCollisionType == Resource::VisualCollisionType::None)
            {
                navigator.addObject(DetourNavigator::ObjectId(object),
                    DetourNavigator::ObjectShapes(object->getShapeInstance(), objectTransform), object->getTransform(),
                    navigatorUpdateGuard);
            }
        }
        else if (physics.getActor(ptr))
        {
            const DetourNavigator::AgentBounds agentBounds = world.getPathfindingAgentBounds(ptr);
            if (!navigator.addAgent(agentBounds))
                Log(Debug::Warning) << "Agent bounds are not supported by navigator: " << agentBounds;
        }
    }

    struct InsertVisitor
    {
        MWWorld::CellStore& mCell;

        std::vector<MWWorld::Ptr> mToInsert;

        InsertVisitor(MWWorld::CellStore& cell);

        bool operator()(const MWWorld::Ptr& ptr);

        template <class AddObject>
        void insert(AddObject&& addObject);
    };

    InsertVisitor::InsertVisitor(MWWorld::CellStore& cell)
        : mCell(cell)
    {
    }

    bool InsertVisitor::operator()(const MWWorld::Ptr& ptr)
    {
        // do not insert directly as we can't modify the cell from within the visitation
        // CreatureLevList::insertObjectRendering may spawn a new creature
        mToInsert.push_back(ptr);
        return true;
    }

    template <class AddObject>
    void InsertVisitor::insert(AddObject&& addObject)
    {
        for (MWWorld::Ptr& ptr : mToInsert)
        {
            if (!ptr.getRefData().isDeleted() && ptr.getRefData().isEnabled())
            {
                try
                {
                    addObject(ptr);
                }
                catch (const std::exception& e)
                {
                    Log(Debug::Error) << "failed to render '" << ptr.getCellRef().getRefId() << "': " << e.what();
                }
            }
        }
    }

    int getCellPositionDistanceToOrigin(const std::pair<int, int>& cellPosition)
    {
        return std::abs(cellPosition.first) + std::abs(cellPosition.second);
    }

    bool isCellInCollection(ESM::ExteriorCellLocation cellIndex, MWWorld::Scene::CellStoreCollection& collection)
    {
        for (auto* cell : collection)
        {
            assert(cell->getCell()->isExterior());
            if (cellIndex == cell->getCell()->getExteriorCellLocation())
                return true;
        }
        return false;
    }

    /*
    bool removeFromSorted(ESM::RefNum refNum, std::vector<ESM::RefNum>& pagedRefs)
    {
        const auto it = std::lower_bound(pagedRefs.begin(), pagedRefs.end(), refNum);
        if (it == pagedRefs.end() || *it != refNum)
            return false;
        pagedRefs.erase(it);
        return true;
    }
    */
}

namespace MWWorld
{

    void Scene::removeFromPagedRefs(const Ptr& ptr)
    {
        /*
        ESM::RefNum refnum = ptr.getCellRef().getRefNum();
        if (refnum.hasContentFile() && removeFromSorted(refnum, mPagedRefs))
        {
            if (!ptr.getRefData().getBaseNode())
                return;
            ptr.getClass().insertObjectRendering(ptr, getModel(ptr), mRendering);
            setNodeRotation(ptr, mRendering, makeNodeRotation(ptr, RotationOrder::direct));
            reloadTerrain();
        }
        */
    }

    void Scene::updateObjectRotation(const Ptr& ptr, RotationOrder order)
    {
        const auto rot = makeNodeRotation(ptr, order);
        setNodeRotation(ptr, mRendering, rot);
        mPhysics->updateRotation(ptr, rot);
    }

    void Scene::updateObjectScale(const Ptr& ptr)
    {
        float scale = ptr.getCellRef().getScale();
        osg::Vec3f scaleVec(scale, scale, scale);
        ptr.getClass().adjustScale(ptr, scaleVec, true);
        mRendering.scaleObject(ptr, scaleVec);
        mPhysics->updateScale(ptr);
    }

    void Scene::update(float duration)
    {
        mPreloader->updateCache();
        preloadCells(duration);
    }

    void Scene::unloadCell(CellStore* cell, const DetourNavigator::UpdateGuard* navigatorUpdateGuard)
    {
        if (mActiveCells.find(cell) == mActiveCells.end())
            return;
        //Log(Debug::Info) << "Unloading cell " << cell->getCell()->getDescription();

        ListAndResetObjectsVisitor visitor;

        cell->forEach(visitor);
        for (const auto& ptr : visitor.mObjects)
        {
            if (const auto object = mPhysics->getObject(ptr))
            {
                if (object->getShapeInstance()->mVisualCollisionType == Resource::VisualCollisionType::None)
                    mNavigator.removeObject(DetourNavigator::ObjectId(object), navigatorUpdateGuard);
                mPhysics->remove(ptr);
                ptr.mRef->mData.mPhysicsPostponed = false;
            }
            else if (mPhysics->getActor(ptr))
            {
                mNavigator.removeAgent(mWorld.getPathfindingAgentBounds(ptr));
                mRendering.removeActorPath(ptr);
                mPhysics->remove(ptr);
            }
            MWBase::Environment::get().getLuaManager()->objectRemovedFromScene(ptr);
        }

        const auto cellX = cell->getCell()->getGridX();
        const auto cellY = cell->getCell()->getGridY();

        if (cell->getCell()->isExterior())
        {
            mNavigator.removeHeightfield(osg::Vec2i(cellX, cellY), navigatorUpdateGuard);
            mPhysics->removeHeightField(cellX, cellY);
        }

        if (cell->getCell()->hasWater())
            mNavigator.removeWater(osg::Vec2i(cellX, cellY), navigatorUpdateGuard);

        if (const auto pathgrid = mWorld.getStore().get<ESM::Pathgrid>().search(*cell->getCell()))
            mNavigator.removePathgrid(*pathgrid);

        MWBase::Environment::get().getMechanicsManager()->drop(cell);

        mRendering.removeCell(cell);
        MWBase::Environment::get().getWindowManager()->removeCell(cell);

        mWorld.getLocalScripts().clearCell(cell);

        MWBase::Environment::get().getSoundManager()->stopSound(cell);
        mActiveCells.erase(cell);
        // Clean up any effects that may have been spawned while unloading all cells
        if (mActiveCells.empty())
            mRendering.notifyWorldSpaceChanged();
    }

    void Scene::loadCell(CellStore& cell, bool respawn,
        const DetourNavigator::UpdateGuard* navigatorUpdateGuard)
    {
        using DetourNavigator::HeightfieldShape;

        assert(mActiveCells.find(&cell) == mActiveCells.end());
        mActiveCells.insert(&cell);

        Log(Debug::Info) << "Loading cell " << cell.getCell()->getDescription();

        const int cellX = cell.getCell()->getGridX();
        const int cellY = cell.getCell()->getGridY();
        const MWWorld::Cell& cellVariant = *cell.getCell();
        ESM::RefId worldspace = cellVariant.getWorldSpace();
        ESM::ExteriorCellLocation cellIndex(cellX, cellY, worldspace);

        if (cellVariant.isExterior())
        {
            auto land = mRendering.getLandManager()->getLand(cellIndex);
            const auto* data = land ? land->getData(ESM::Land::DATA_VHGT) : nullptr;
            const int verts = ESM::Land::LAND_SIZE;
            const int worldsize = ESM::Land::REAL_SIZE;
            if (data)
            {
                mPhysics->addHeightField(data->mHeights, cellX, cellY, worldsize, verts,
                    data->mMinHeight, data->mMaxHeight, land.get());
            }
            else if (!ESM::isEsm4Ext(worldspace))
            {
                static std::vector<float> defaultHeight;
                defaultHeight.resize(verts * verts, ESM::Land::DEFAULT_HEIGHT);
                mPhysics->addHeightField(defaultHeight.data(), cellX, cellY, worldsize, verts,
                    ESM::Land::DEFAULT_HEIGHT, ESM::Land::DEFAULT_HEIGHT, land.get());
            }
            if (const auto heightField = mPhysics->getHeightField(cellX, cellY))
            {
                const osg::Vec2i cellPosition(cellX, cellY);
                const btVector3& origin = heightField->getCollisionObject()->getWorldTransform().getOrigin();
                const osg::Vec3f shift(origin.x(), origin.y(), origin.z());
                const HeightfieldShape shape = [&]() -> HeightfieldShape {
                    if (data == nullptr)
                    {
                        return DetourNavigator::HeightfieldPlane{ static_cast<float>(ESM::Land::DEFAULT_HEIGHT) };
                    }
                    else
                    {
                        DetourNavigator::HeightfieldSurface heights;
                        heights.mHeights = data->mHeights;
                        heights.mSize = static_cast<std::size_t>(Constants::CellSizeInUnits);
                        heights.mMinHeight = data->mMinHeight;
                        heights.mMaxHeight = data->mMaxHeight;
                        return heights;
                    }
                }();
                mNavigator.addHeightfield(cellPosition, worldsize, shape, navigatorUpdateGuard);
            }
        }

        /*
        ESM::visit(ESM::VisitOverload{
                       [&](const ESM::Cell& cell) {
                           if (const auto pathgrid = mWorld.getStore().get<ESM::Pathgrid>().search(cell))
                               mNavigator.addPathgrid(cell, *pathgrid);
                       },
                       [&](const ESM4::Cell& cell) {},
                   },
            *cell.getCell());

            */
        if (const auto pathgrid = mWorld.getStore().get<ESM::Pathgrid>().search(*cell.getCell()))
            mNavigator.addPathgrid(cell.getCell()->getEsm3(), *pathgrid);

        // register local scripts
        // do this before insertCell, to make sure we don't add scripts from levelled creature spawning twice
        mWorld.getLocalScripts().addCell(&cell);

        if (respawn)
            cell.respawn();

        insertCell(cell, navigatorUpdateGuard);

        mRendering.addCell(&cell);

        MWBase::Environment::get().getWindowManager()->addCell(&cell);
        bool waterEnabled = cellVariant.hasWater() || cell.isExterior();
        float waterLevel = cell.getWaterLevel();
        mRendering.setWaterEnabled(waterEnabled);
        if (waterEnabled)
        {
            mPhysics->enableWater(waterLevel);
            mRendering.setWaterHeight(waterLevel);

            if (cellVariant.isExterior())
            {
                if (const auto heightField = mPhysics->getHeightField(cellX, cellY))
                    mNavigator.addWater(
                        osg::Vec2i(cellX, cellY), ESM::Land::REAL_SIZE, waterLevel, navigatorUpdateGuard);
            }
            else
            {
                mNavigator.addWater(
                    osg::Vec2i(cellX, cellY), std::numeric_limits<int>::max(), waterLevel, navigatorUpdateGuard);
            }
        }
        else
            mPhysics->disableWater();

        if (!cell.isExterior() && !cellVariant.isQuasiExterior())
            mRendering.configureAmbient(cellVariant);
    }

    void Scene::clear()
    {
        auto navigatorUpdateGuard = mNavigator.makeUpdateGuard();
        for (auto iter = mActiveCells.begin(); iter != mActiveCells.end();)
        {
            auto* cell = *iter++;
            unloadCell(cell, navigatorUpdateGuard.get());
        }
        navigatorUpdateGuard.reset();
        assert(mActiveCells.empty());
        mCurrentCell = nullptr;

        mPreloader->clear();
    }

    osg::Vec4i Scene::gridCenterToBounds(const osg::Vec2i& centerCell) const
    {
        return osg::Vec4i(centerCell.x() - mHalfGridSize, centerCell.y() - mHalfGridSize,
            centerCell.x() + mHalfGridSize + 1, centerCell.y() + mHalfGridSize + 1);
    }

    osg::Vec2i Scene::getNewGridCenter(const osg::Vec3f& pos, const osg::Vec2i* currentGridCenter) const
    {
        ESM::RefId worldspace
            = mCurrentCell ? mCurrentCell->getCell()->getWorldSpace() : ESM::Cell::sDefaultWorldspaceId;
        if (currentGridCenter)
        {
            osg::Vec2 center = ESM::indexToPosition(
                ESM::ExteriorCellLocation(currentGridCenter->x(), currentGridCenter->y(), worldspace), true);
            float distance = std::max(std::abs(center.x() - pos.x()), std::abs(center.y() - pos.y()));
            float cellSize = ESM::getCellSize(worldspace);
            const float maxDistance = cellSize / 2 + mCellLoadingThreshold; // 1/2 cell size + threshold
            if (distance <= maxDistance)
                return *currentGridCenter;
        }
        ESM::ExteriorCellLocation cellPos = ESM::positionToExteriorCellLocation(pos.x(), pos.y(), worldspace);
        return { cellPos.mX, cellPos.mY };
    }

    bool Scene::needToChangeCellGrid(const osg::Vec3f& pos)
    {
        if (!mCurrentCell || !mCurrentCell->isExterior())
            return false;

        osg::Vec2i newCell = getNewGridCenter(pos, &mCurrentGridCenter);
        return newCell != mCurrentGridCenter;
    }

    void Scene::changeCellGrid(const osg::Vec3f& pos, ESM::ExteriorCellLocation playerCellIndex, bool changeEvent)
    {
        mHalfGridSize
            = /*isEsm4Ext(playerCellIndex.mWorldspace) ? Constants::ESM4CellGridRadius : */Constants::CellGridRadius;
        auto navigatorUpdateGuard = mNavigator.makeUpdateGuard();
        int playerCellX = playerCellIndex.mX;
        int playerCellY = playerCellIndex.mY;

        for (auto iter = mActiveCells.begin(); iter != mActiveCells.end();)
        {
            auto* cell = *iter++;
            if (cell->getCell()->isExterior() && cell->getCell()->getWorldSpace() == playerCellIndex.mWorldspace)
            {
                const auto dx = std::abs(playerCellX - cell->getCell()->getGridX());
                const auto dy = std::abs(playerCellY - cell->getCell()->getGridY());
                if (dx > mHalfGridSize || dy > mHalfGridSize)
                    unloadCell(cell, navigatorUpdateGuard.get());
            }
            else
                unloadCell(cell, navigatorUpdateGuard.get());
        }
        mNavigator.setWorldspace(
            mWorld.getWorldModel().getExterior(playerCellIndex).getCell()->getWorldSpace().serializeText(),
            navigatorUpdateGuard.get());
        mNavigator.updateBounds(pos, navigatorUpdateGuard.get());

        mCurrentGridCenter = osg::Vec2i(playerCellX, playerCellY);
        mRendering.setRenderModeActive(MWRender::Render_Terrain, true);
        mRendering.changeCellGrid(pos);

        /*
        if (mRendering.pagingUnlockCache())
            mPreloader->abortTerrainPreloadExcept(nullptr);
        if (!mPreloader->isTerrainLoaded(std::make_pair(pos, newGrid), mRendering.getReferenceTime()))
            preloadTerrain(pos, playerCellIndex.mWorldspace, true);
        mPagedRefs.clear();
        mRendering.getPagedRefnums(newGrid, mPagedRefs);
        */

        std::size_t refsToLoad = 0;
        const auto cellsToLoad = [&](CellStoreCollection& collection, int range) -> std::vector<std::pair<int, int>> {
            std::vector<std::pair<int, int>> cellsPositionsToLoad;
            for (int x = playerCellX - range; x <= playerCellX + range; ++x)
            {
                for (int y = playerCellY - range; y <= playerCellY + range; ++y)
                {
                    if (!isCellInCollection(ESM::ExteriorCellLocation(x, y, playerCellIndex.mWorldspace), collection))
                    {
                        refsToLoad += mWorld.getWorldModel().getExterior(playerCellIndex).count();
                        cellsPositionsToLoad.emplace_back(x, y);
                    }
                }
            }
            return cellsPositionsToLoad;
        };

        addPostponedPhysicsObjects();

        auto cellsPositionsToLoad = cellsToLoad(mActiveCells, mHalfGridSize);

        const auto getDistanceToPlayerCell = [&](const std::pair<int, int>& cellPosition) {
            return std::abs(cellPosition.first - playerCellX) + std::abs(cellPosition.second - playerCellY);
        };

        const auto getCellPositionPriority = [&](const std::pair<int, int>& cellPosition) {
            return std::make_pair(getDistanceToPlayerCell(cellPosition), getCellPositionDistanceToOrigin(cellPosition));
        };

        std::sort(cellsPositionsToLoad.begin(), cellsPositionsToLoad.end(),
            [&](const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) {
                return getCellPositionPriority(lhs) < getCellPositionPriority(rhs);
            });

        for (const auto& [x, y] : cellsPositionsToLoad)
        {
            ESM::ExteriorCellLocation indexToLoad = { x, y, playerCellIndex.mWorldspace };
            if (!isCellInCollection(indexToLoad, mActiveCells))
            {
                CellStore& cell = mWorld.getWorldModel().getExterior(indexToLoad);
                loadCell(cell, changeEvent, navigatorUpdateGuard.get());
            }
        }

        mNavigator.update(pos, navigatorUpdateGuard.get());

        navigatorUpdateGuard.reset();

        CellStore& current = mWorld.getWorldModel().getExterior(playerCellIndex);
        MWBase::Environment::get().getWindowManager()->changeCell(&current);

        if (changeEvent)
            mCellChanged = true;

        mCellLoaded = true;
    }

    void Scene::addPostponedPhysicsObjects()
    {
        for (const auto& cell : mActiveCells)
        {
            cell->forEach([&](const MWWorld::Ptr& ptr) {
                if (ptr.mRef->mData.mPhysicsPostponed)
                {
                    ptr.mRef->mData.mPhysicsPostponed = false;
                    if (ptr.mRef->mData.isEnabled() && ptr.mRef->mData.getCount() > 0)
                    {
                        std::string model = getModel(ptr);
                        if (!model.empty())
                        {
                            const auto rotation = makeNodeRotation(ptr, RotationOrder::direct);
                            ptr.getClass().insertObjectPhysics(ptr, model, rotation, *mPhysics);
                        }
                    }
                }
                return true;
            });
        }
    }

    void Scene::testExteriorCells(MWState::Loading& state)
    {
        const MWWorld::Store<ESM::Cell>& cells = mWorld.getStore().get<ESM::Cell>();

        state.progressStep = 1.f/cells.getExtSize();

        MWWorld::Store<ESM::Cell>::iterator it = cells.extBegin();
        int i = 1;
        auto navigatorUpdateGuard = mNavigator.makeUpdateGuard();
        for (; !state.abort && it != cells.extEnd(); ++it)
        {
            state.setDescription("#{OMWEngine:TestingExteriorCells} (" + std::to_string(i) + "/"
                + std::to_string(cells.getExtSize()) + ")...");

            CellStore& cell = mWorld.getWorldModel().getExterior(
                ESM::ExteriorCellLocation(it->mData.mX, it->mData.mY, ESM::Cell::sDefaultWorldspaceId));
            mNavigator.setWorldspace(cell.getCell()->getWorldSpace().serializeText(), navigatorUpdateGuard.get());
            const osg::Vec3f position
                = osg::Vec3f(it->mData.mX + 0.5f, it->mData.mY + 0.5f, 0) * Constants::CellSizeInUnits;
            mNavigator.updateBounds(position, navigatorUpdateGuard.get());
            loadCell(cell, false, navigatorUpdateGuard.get());

            mNavigator.update(position, navigatorUpdateGuard.get());
            navigatorUpdateGuard.reset();
            mNavigator.wait(DetourNavigator::WaitConditionType::requiredTilesPresent, nullptr);
            navigatorUpdateGuard = mNavigator.makeUpdateGuard();

            auto iter = mActiveCells.begin();
            while (iter != mActiveCells.end())
            {
                if (it->isExterior() && it->mData.mX == (*iter)->getCell()->getGridX()
                    && it->mData.mY == (*iter)->getCell()->getGridY())
                {
                    unloadCell(*iter, navigatorUpdateGuard.get());
                    break;
                }

                ++iter;
            }

            mRendering.pruneCache();
            mPhysics->pruneCache();

            state.advance();
            i++;
        }
    }

    void Scene::testInteriorCells(MWState::Loading& state)
    {
        const MWWorld::Store<ESM::Cell>& cells = mWorld.getStore().get<ESM::Cell>();

        state.progressStep = 1.f/cells.getIntSize();

        int i = 1;
        MWWorld::Store<ESM::Cell>::iterator it = cells.intBegin();
        auto navigatorUpdateGuard = mNavigator.makeUpdateGuard();
        for (; !state.abort && it != cells.intEnd(); ++it)
        {
            state.setDescription("#{OMWEngine:TestingInteriorCells} (" + std::to_string(i) + "/"
                + std::to_string(cells.getIntSize()) + ")...");

            CellStore& cell = mWorld.getWorldModel().getInterior(it->mName);
            mNavigator.setWorldspace(cell.getCell()->getWorldSpace().serializeText(), navigatorUpdateGuard.get());
            ESM::Position position;
            mWorld.findInteriorPosition(it->mName, position);
            mNavigator.updateBounds(position.asVec3(), navigatorUpdateGuard.get());
            loadCell(cell, false, navigatorUpdateGuard.get());

            mNavigator.update(position.asVec3(), navigatorUpdateGuard.get());
            navigatorUpdateGuard.reset();
            mNavigator.wait(DetourNavigator::WaitConditionType::requiredTilesPresent, nullptr);
            navigatorUpdateGuard = mNavigator.makeUpdateGuard();

            auto iter = mActiveCells.begin();
            while (iter != mActiveCells.end())
            {
                assert(!(*iter)->getCell()->isExterior());

                if (it->mName == (*iter)->getCell()->getNameId())
                {
                    unloadCell(*iter, navigatorUpdateGuard.get());
                    break;
                }

                ++iter;
            }

            mRendering.pruneCache();
            mPhysics->pruneCache();

            state.advance();
            i++;
        }
    }

    void Scene::changePlayerCell(CellStore& cell)
    {
        mHalfGridSize = /*cell.getCell()->isEsm4() ? Constants::ESM4CellGridRadius : */Constants::CellGridRadius;
        mCurrentCell = &cell;

        MWWorld::Ptr old = mWorld.getPlayerPtr();
        mWorld.removeContainerScripts(old);
        mWorld.getPlayer().setCell(&cell);

        MWWorld::Ptr player = mWorld.getPlayerPtr();
        mRendering.updatePlayerPtr(player);
        mWorld.addContainerScripts(player, &cell);

        // The player is loaded before the scene and by default it is grounded, with the scene fully loaded,
        // we validate and correct this. Only run once, during initial cell load.
        if (old.mCell == &cell)
            mPhysics->traceDown(player, player.getRefData().getPosition().asVec3(), 10.f);

        MWBase::Environment::get().getMechanicsManager()->updateCell(old, player);
        MWBase::Environment::get().getWindowManager()->watchActor(player);

        mPhysics->updatePtr(old, player);

        mWorld.adjustSky();

        mLastPlayerPos = player.getRefData().getPosition().asVec3();

        mRendering.pruneCache();
        mPhysics->pruneCache();
    }

    Scene::Scene(MWWorld::World& world, MWRender::RenderManager& rendering, MWPhysics::PhysicsSystem* physics,
        DetourNavigator::Navigator& navigator)
        : mCurrentCell(nullptr)
        , mCellChanged(false)
        , mWorld(world)
        , mPhysics(physics)
        , mRendering(rendering)
        , mNavigator(navigator)
        , mCellLoadingThreshold(1024.f)
        , mPreloadDistance(Settings::cells().mPreloadDistance)
        , mPreloadEnabled(Settings::cells().mPreloadEnabled)
        , mPreloadExteriorGrid(Settings::cells().mPreloadExteriorGrid)
        , mPreloadDoors(Settings::cells().mPreloadDoors)
        , mPreloadFastTravel(Settings::cells().mPreloadFastTravel)
        , mPredictionTime(Settings::cells().mPredictionTime)
    {
        mPreloader = std::make_unique<CellPreloader>(rendering.getAnimContext(), mPhysics->actorReadOptions(), mPhysics->readOptions(), rendering.getLandManager(), rendering.getOperationThreads());
        mTerrainPreloader = std::make_unique<Terrain::Preload>(rendering.getTerrain(), rendering.getOperationThreads(), mPreloader->getCompileThreads());

        //rendering.getResourceSystem()->setExpiryDelay(Settings::Manager::getFloat("cache expiry delay", "Cells"));

        mPreloader->setMinCacheSize(Settings::cells().mPreloadCellCacheMin);
        mPreloader->setMaxCacheSize(Settings::cells().mPreloadCellCacheMax);
    }

    Scene::~Scene()
    {
        /*
        for (const osg::ref_ptr<SceneUtil::WorkItem>& v : mWorkItems)
            v->abort();

        for (const osg::ref_ptr<SceneUtil::WorkItem>& v : mWorkItems)
            v->waitTillDone();
            */
    }

    bool Scene::hasCellChanged() const
    {
        return mCellChanged;
    }

    const Scene::CellStoreCollection& Scene::getActiveCells() const
    {
        return mActiveCells;
    }

    void Scene::changeToInteriorCell(std::string_view cellName, const ESM::Position& position, bool changeEvent)
    {
        CellStore& cell = mWorld.getWorldModel().getInterior(cellName);

        if (mCurrentCell == &cell)
        {
            mWorld.moveObject(mWorld.getPlayerPtr(), position.asVec3());
            mWorld.rotateObject(mWorld.getPlayerPtr(), position.asRotationVec3());
            return;
        }

        Log(Debug::Info) << "Changing to interior";

        auto navigatorUpdateGuard = mNavigator.makeUpdateGuard();

        // unload
        for (auto iter = mActiveCells.begin(); iter != mActiveCells.end();)
        {
            auto* cellToUnload = *iter++;
            unloadCell(cellToUnload, navigatorUpdateGuard.get());
        }
        assert(mActiveCells.empty());

        mNavigator.setWorldspace(cell.getCell()->getWorldSpace().serializeText(), navigatorUpdateGuard.get());
        mNavigator.updateBounds(position.asVec3(), navigatorUpdateGuard.get());

        // Load cell.
        mPagedRefs.clear();
        loadCell(cell, changeEvent, navigatorUpdateGuard.get());

        navigatorUpdateGuard.reset();

        changePlayerCell(cell);

        // adjust fog
        mRendering.configureFog(*mCurrentCell->getCell());

        mRendering.setRenderModeActive(MWRender::Render_Terrain, false);

        // Sky system
        mWorld.adjustSky();

        if (changeEvent)
            mCellChanged = true;

        mCellLoaded = true;

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);

        //MWBase::Environment::get().getWorld()->getPostProcessor()->setExteriorFlag(cell.getCell()->isQuasiExterior());
    }

    void Scene::changeToExteriorCell(
        const ESM::RefId& extCellId, const ESM::Position& position, bool changeEvent)
    {
        ///MWBase::Environment::get().getWorld()->getPostProcessor()->setExteriorFlag(true);

        CellStore& current = mWorld.getWorldModel().getCell(extCellId);
        const osg::Vec2i cellIndex(current.getCell()->getGridX(), current.getCell()->getGridY());
        changeCellGrid(position.asVec3(),
            ESM::ExteriorCellLocation(cellIndex.x(), cellIndex.y(), current.getCell()->getWorldSpace()), changeEvent);

        if (&current != mCurrentCell)
            changePlayerCell(current);
    }

    CellStore* Scene::getCurrentCell()
    {
        return mCurrentCell;
    }

    void Scene::markCellAsUnchanged()
    {
        mCellChanged = false;
    }

    void Scene::insertCell(
        CellStore& cell, const DetourNavigator::UpdateGuard* navigatorUpdateGuard)
    {
        InsertVisitor insertVisitor(cell);
        cell.forEach(insertVisitor);
        insertVisitor.insert(
            [&](const MWWorld::Ptr& ptr) { addObject(ptr, mWorld, mPagedRefs, *mPhysics, mRendering); });
        insertVisitor.insert(
            [&](const MWWorld::Ptr& ptr) { addObject(ptr, mWorld, *mPhysics, mNavigator, navigatorUpdateGuard); });
    }

    void Scene::addObjectToScene(const Ptr& ptr)
    {
        try
        {
            addObject(ptr, mWorld, mPagedRefs, *mPhysics, mRendering);
            addObject(ptr, mWorld, *mPhysics, mNavigator);
            mWorld.scaleObject(ptr, ptr.getCellRef().getScale());
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "failed to render '" << ptr.getCellRef().getRefId() << "': " << e.what();
        }
    }

    void Scene::removeObjectFromScene(const Ptr& ptr, bool keepActive)
    {
        MWBase::Environment::get().getMechanicsManager()->remove(ptr, keepActive);
        // You'd expect the sounds attached to the object to be stopped here
        // because the object is nowhere to be heard, but in Morrowind, they're not.
        // They're still stopped when the cell is unloaded
        // or if the player moves away far from the object's position.
        // Todd Howard, Who art in Bethesda, hallowed be Thy name.
        MWBase::Environment::get().getLuaManager()->objectRemovedFromScene(ptr);
        if (const auto object = mPhysics->getObject(ptr))
        {
            if (object->getShapeInstance()->mVisualCollisionType == Resource::VisualCollisionType::None)
                mNavigator.removeObject(DetourNavigator::ObjectId(object), nullptr);
        }
        else if (mPhysics->getActor(ptr))
        {
            mNavigator.removeAgent(mWorld.getPathfindingAgentBounds(ptr));
        }
        mPhysics->remove(ptr);
        mRendering.removeObject(ptr);
        if (ptr.getClass().isActor())
            mRendering.removeWaterRippleEmitter(ptr);
    }

    bool Scene::isCellActive(const CellStore& cell)
    {
        return mActiveCells.contains(const_cast<CellStore*>(&cell));
    }

    Ptr Scene::searchPtrViaActorId(int actorId)
    {
        for (CellStoreCollection::const_iterator iter(mActiveCells.begin()); iter != mActiveCells.end(); ++iter)
        {
            Ptr ptr = (*iter)->searchViaActorId(actorId);
            if (!ptr.isEmpty())
                return ptr;
        }
        return Ptr();
    }

    /*
    class PreloadMeshItem : public SceneUtil::WorkItem
    {
    public:
        PreloadMeshItem(const std::string& mesh, Resource::SceneManager* sceneManager)
            : mMesh(mesh), mSceneManager(sceneManager)
        {
        }

        void doWork() override
        {
            if (mAborted)
                return;

            try
            {
                mSceneManager->getTemplate(mMesh);
            }
            catch (std::exception&)
            {
            }
        }

        void abort() override
        {
            mAborted = true;
        }

    private:
        std::string mMesh;
        Resource::SceneManager* mSceneManager;
        std::atomic_bool mAborted {false};
    };
    */

    void Scene::preload(const std::string& mesh, bool useAnim)
    {
        /*
        std::string mesh_ = mesh;
        if (useAnim)
            mesh_ = Misc::ResourceHelpers::correctActorModelPath(mesh_, mRendering.getResourceSystem()->getVFS());

        if (!mRendering.getResourceSystem()->getSceneManager()->checkLoaded(mesh_, mRendering.getReferenceTime()))
        {
            osg::ref_ptr<PreloadMeshItem> item(new PreloadMeshItem(mesh_,
        mRendering.getResourceSystem()->getSceneManager())); mRendering.getWorkQueue()->addWorkItem(item); const auto
        isDone = [] (const osg::ref_ptr<SceneUtil::WorkItem>& v) { return v->isDone(); };
            mWorkItems.erase(std::remove_if(mWorkItems.begin(), mWorkItems.end(), isDone), mWorkItems.end());
            mWorkItems.emplace_back(std::move(item));
        }
        */
    }

    void Scene::preloadCells(float dt)
    {
        if (dt<=1e-06) return;
        std::vector<PositionCellGrid> exteriorPositions;

        const MWWorld::ConstPtr player = mWorld.getPlayerPtr();
        osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();
        osg::Vec3f moved = playerPos - mLastPlayerPos;
        osg::Vec3f predictedPos = playerPos + moved / dt * mPredictionTime;

        if (mCurrentCell->isExterior())
            exteriorPositions.emplace_back(predictedPos, gridCenterToBounds(getNewGridCenter(predictedPos, &mCurrentGridCenter)));

        mLastPlayerPos = playerPos;

        if (mPreloadEnabled)
        {
            if (mPreloadDoors)
                preloadTeleportDoorDestinations(playerPos, predictedPos, exteriorPositions);
            if (mPreloadExteriorGrid)
                preloadExteriorGrid(playerPos, predictedPos);
            if (mPreloadFastTravel)
                preloadFastTravelDestinations(playerPos, predictedPos, exteriorPositions);
        }

        std::vector<Terrain::Preload::Position> terrainPositions;
        for (const auto& p : exteriorPositions)
            terrainPositions.emplace_back(vsg::dvec3(toVsg(p.first)), mRendering.getViewDistance());
        mTerrainPreloader->setPositions(terrainPositions);
    }

    void Scene::preloadTeleportDoorDestinations(
        const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos, std::vector<PositionCellGrid>& exteriorPositions)
    {
        std::vector<MWWorld::ConstPtr> teleportDoors;
        for (const MWWorld::CellStore* cellStore : mActiveCells)
        {
            typedef MWWorld::CellRefList<ESM::Door>::List DoorList;
            const DoorList& doors = cellStore->getReadOnlyDoors().mList;
            for (auto& door : doors)
            {
                if (!door.mRef.getTeleport())
                {
                    continue;
                }
                teleportDoors.emplace_back(&door, cellStore);
            }
        }

        for (const MWWorld::ConstPtr& door : teleportDoors)
        {
            float sqrDistToPlayer = (playerPos - door.getRefData().getPosition().asVec3()).length2();
            sqrDistToPlayer
                = std::min(sqrDistToPlayer, (predictedPos - door.getRefData().getPosition().asVec3()).length2());

            if (sqrDistToPlayer < mPreloadDistance * mPreloadDistance)
            {
                try
                {
                    preloadCell(mWorld.getWorldModel().getCell(door.getCellRef().getDestCell()));
                }
                catch (std::exception&)
                {
                    // ignore error for now, would spam the log too much
                }
            }
        }
    }

    void Scene::preloadExteriorGrid(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos)
    {
        if (!mWorld.isCellExterior())
            return;

        int halfGridSizePlusOne = mHalfGridSize + 1;

        int cellX, cellY;
        cellX = mCurrentGridCenter.x();
        cellY = mCurrentGridCenter.y();
        ESM::RefId extWorldspace = mWorld.getCurrentWorldspace();

        float cellSize = ESM::getCellSize(extWorldspace);

        for (int dx = -halfGridSizePlusOne; dx <= halfGridSizePlusOne; ++dx)
        {
            for (int dy = -halfGridSizePlusOne; dy <= halfGridSizePlusOne; ++dy)
            {
                if (dy != halfGridSizePlusOne && dy != -halfGridSizePlusOne && dx != halfGridSizePlusOne
                    && dx != -halfGridSizePlusOne)
                    continue; // only care about the outer (not yet loaded) part of the grid
                ESM::ExteriorCellLocation cellIndex(cellX + dx, cellY + dy, extWorldspace);
                osg::Vec2 thisCellCenter = ESM::indexToPosition(cellIndex, true);

                float dist = std::max(
                    std::abs(thisCellCenter.x() - playerPos.x()), std::abs(thisCellCenter.y() - playerPos.y()));
                dist = std::min(dist,
                    std::max(std::abs(thisCellCenter.x() - predictedPos.x()),
                        std::abs(thisCellCenter.y() - predictedPos.y())));
                float loadDist = cellSize / 2 + cellSize - mCellLoadingThreshold + mPreloadDistance;

                if (dist < loadDist)
                    preloadCell(mWorld.getWorldModel().getExterior(cellIndex));
            }
        }
    }

    float /*progress*/ Scene::preloadCell(CellStore& cell, bool preloadSurrounding)
    {
        float progress = 0;
        unsigned int numpreloaded = 0;
        if (preloadSurrounding && cell.isExterior())
        {
            int x = cell.getCell()->getGridX();
            int y = cell.getCell()->getGridY();
            for (int dx = -mHalfGridSize; dx <= mHalfGridSize; ++dx)
            {
                for (int dy = -mHalfGridSize; dy <= mHalfGridSize; ++dy)
                {
                    auto& c = mWorld.getWorldModel().getExterior(ESM::ExteriorCellLocation(x + dx, y + dy, cell.getCell()->getWorldSpace()));
                    if (mActiveCells.find(&c) != mActiveCells.end())
                        continue;
                    progress += mPreloader->preload(c);
                    if (++numpreloaded >= mPreloader->getMaxCacheSize())
                        break;
                }
            }
        }
        else
            progress = mPreloader->preload(cell);
        if (numpreloaded)
            progress /= numpreloaded;
        return progress;
    }

    void Scene::preloadTerrain(const osg::Vec3f& pos, ESM::RefId worldspace, bool sync)
    {
        std::vector<Terrain::Preload::Position> vec;
        vec.emplace_back(vsg::dvec3(toVsg(pos)), mRendering.getViewDistance());  //gridCenterToBounds(getNewGridCenter(pos)));
        //mPreloader->abortTerrainPreloadExcept(vec.data());
        mTerrainPreloader->setPositions(vec);
        /*
        if (!sync) return;

        loadingListener->setLabel("#{OMWEngine:InitializingData}");

        mPreloader->syncTerrainLoad(*loadingListener);
        */
    }

    void Scene::reloadTerrain()
    {
        // mPreloader->setTerrainPreloadPositions(std::vector<PositionCellGrid>());
    }

    struct ListFastTravelDestinationsVisitor
    {
        ListFastTravelDestinationsVisitor(float preloadDist, const osg::Vec3f& playerPos)
            : mPreloadDist(preloadDist)
            , mPlayerPos(playerPos)
        {
        }

        bool operator()(const MWWorld::Ptr& ptr)
        {
            if ((ptr.getRefData().getPosition().asVec3() - mPlayerPos).length2() > mPreloadDist * mPreloadDist)
                return true;

            if (ptr.getClass().isNpc())
            {
                const std::vector<ESM::Transport::Dest>& transport = ptr.get<ESM::NPC>()->mBase->mTransport.mList;
                mList.insert(mList.begin(), transport.begin(), transport.end());
            }
            else
            {
                const std::vector<ESM::Transport::Dest>& transport = ptr.get<ESM::Creature>()->mBase->mTransport.mList;
                mList.insert(mList.begin(), transport.begin(), transport.end());
            }
            return true;
        }
        float mPreloadDist;
        osg::Vec3f mPlayerPos;
        std::vector<ESM::Transport::Dest> mList;
    };

    void Scene::preloadFastTravelDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& /*predictedPos*/,
        std::vector<PositionCellGrid>& exteriorPositions) // ignore predictedPos here since opening dialogue with
                                                          // travel service takes extra time
    {
        const MWWorld::ConstPtr player = mWorld.getPlayerPtr();
        ListFastTravelDestinationsVisitor listVisitor(mPreloadDistance, player.getRefData().getPosition().asVec3());
        ESM::RefId extWorldspace = mWorld.getCurrentWorldspace();
        for (MWWorld::CellStore* cellStore : mActiveCells)
        {
            cellStore->forEachType<ESM::NPC>(listVisitor);
            cellStore->forEachType<ESM::Creature>(listVisitor);
        }

        for (ESM::Transport::Dest& dest : listVisitor.mList)
        {
            if (!dest.mCellName.empty())
                preloadCell(mWorld.getWorldModel().getInterior(dest.mCellName));
            else
            {
                osg::Vec3f pos = dest.mPos.asVec3();
                const ESM::ExteriorCellLocation cellIndex
                    = ESM::positionToExteriorCellLocation(pos.x(), pos.y(), extWorldspace);
                preloadCell(mWorld.getWorldModel().getExterior(cellIndex), true);
                exteriorPositions.emplace_back(pos, gridCenterToBounds(getNewGridCenter(pos)));
            }
        }
    }
}
