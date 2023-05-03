#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include <osg/Vec2i>
#include <osg/Vec4i>
#include <osg/ref_ptr>

#include "ptr.hpp"

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include <components/misc/constants.hpp>

namespace osg
{
    class Vec3f;
}

namespace ESM
{
    struct Position;
}

namespace Files
{
    class Collections;
}

namespace DetourNavigator
{
    struct Navigator;
    class UpdateGuard;
}

namespace MWState
{
    class Loading;
}
namespace MWRender
{
    class RenderManager;
}

namespace MWPhysics
{
    class PhysicsSystem;
}

namespace MWWorld
{
    class Player;
    class CellStore;
    class CellPreloader;
    class World;

    enum class RotationOrder
    {
        direct,
        inverse
    };

    class Scene
    {
    public:
        using CellStoreCollection = std::set<CellStore*>;

    private:
        CellStore* mCurrentCell; // the cell the player is in
        CellStoreCollection mActiveCells;
        bool mCellChanged;
        bool mCellLoaded = false;
        MWWorld::World& mWorld;
        MWPhysics::PhysicsSystem* mPhysics;
        MWRender::RenderManager& mRendering;
        DetourNavigator::Navigator& mNavigator;
        std::unique_ptr<CellPreloader> mPreloader;
        float mCellLoadingThreshold;
        float mPreloadDistance;
        bool mPreloadEnabled;

        bool mPreloadExteriorGrid;
        bool mPreloadDoors;
        bool mPreloadFastTravel;
        float mPredictionTime;

        static const int mHalfGridSize = Constants::CellGridRadius;

        osg::Vec3f mLastPlayerPos;

        std::vector<ESM::RefNum> mPagedRefs;

        void insertCell(CellStore& cell,
            const DetourNavigator::UpdateGuard* navigatorUpdateGuard);

        osg::Vec2i mCurrentGridCenter;

        // Load and unload cells as necessary to create a cell grid with "X" and "Y" in the center
        void changeCellGrid(const osg::Vec3f& pos, int playerCellX, int playerCellY, bool changeEvent = true);

        typedef std::pair<osg::Vec3f, osg::Vec4i> PositionCellGrid;

        void preloadCells(float dt);
        void preloadTeleportDoorDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos,
            std::vector<PositionCellGrid>& exteriorPositions);
        void preloadExteriorGrid(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos);
        void preloadFastTravelDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos,
            std::vector<PositionCellGrid>& exteriorPositions);

        osg::Vec4i gridCenterToBounds(const osg::Vec2i& centerCell) const;
        osg::Vec2i getNewGridCenter(const osg::Vec3f& pos, const osg::Vec2i* currentGridCenter = nullptr) const;

        void unloadCell(CellStore* cell, const DetourNavigator::UpdateGuard* navigatorUpdateGuard);
        void loadCell(CellStore* cell, bool respawn, const DetourNavigator::UpdateGuard* navigatorUpdateGuard);

    public:
        Scene(MWWorld::World& world, MWRender::RenderManager& rendering, MWPhysics::PhysicsSystem* physics,
            DetourNavigator::Navigator& navigator);

        ~Scene();

        float /*progress*/  preloadCell(MWWorld::CellStore* cell, bool preloadSurrounding = false);
        float /*progress*/  preloadCell(const ESM::CellId& cell, bool preloadSurrounding = false);
        void preloadTerrain(const osg::Vec3f& pos, bool sync = false);
        void reloadTerrain();

        bool needToChangeCellGrid(const osg::Vec3f& playerPos);

        void changePlayerCell(CellStore* newCell);

        CellStore* getCurrentCell();

        const CellStoreCollection& getActiveCells() const;

        bool hasCellChanged() const;
        ///< Has the set of active cells changed, since the last frame?

        bool hasCellLoaded() const { return mCellLoaded; }

        void resetCellLoaded() { mCellLoaded = false; }

        void changeToInteriorCell(std::string_view cellName, const ESM::Position& position, bool changeEvent = true);
        ///< Move to interior cell.
        /// @param changeEvent Set cellChanged flag?

        void changeToExteriorCell(const ESM::Position& position, bool changeEvent = true);
        ///< Move to exterior cell.
        /// @param changeEvent Set cellChanged flag?

        void clear();
        ///< Change into a void

        void markCellAsUnchanged();

        void update(float duration);

        void addObjectToScene(const Ptr& ptr);
        ///< Add an object that already exists in the world model to the scene.

        void removeObjectFromScene(const Ptr& ptr, bool keepActive = false);
        ///< Remove an object from the scene, but not from the world model.

        void addPostponedPhysicsObjects();

        void removeFromPagedRefs(const Ptr& ptr);

        void updateObjectRotation(const Ptr& ptr, RotationOrder order);
        void updateObjectScale(const Ptr& ptr);

        bool isCellActive(const CellStore& cell);

        Ptr searchPtrViaActorId(int actorId);

        void preload(const std::string& mesh, bool useAnim = false);

        void testExteriorCells(MWState::Loading& state);
        void testInteriorCells(MWState::Loading& state);
    };
}

#endif
