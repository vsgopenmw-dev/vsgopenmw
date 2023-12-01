#ifndef VSGOPENMW_MWRENDER_RENDERMANAGER_H
#define VSGOPENMW_MWRENDER_RENDERMANAGER_H

#include "renderinginterface.hpp"
#include "rendermode.hpp"

#include <deque>
#include <memory>
#include <optional>

#include <vsg/nodes/Group.h>
#include <vsg/maths/box.h>

#include <osg/BoundingBox>
#include <osg/Quat>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osg/Vec4i>

#include <components/settings/settings.hpp>

namespace MWWorld
{
    class Ptr;
    class ConstPtr;
    class CellStore;
    class Cell;
    class ESMStore;
}
namespace Resource
{
    class ResourceSystem;
}
namespace Render
{
    class Engine;
}
namespace vsg
{
    class Camera;
    class LookAt;
    class Perspective;
    class View;
    class RecordTraversal;
    class OperationThreads;
}
namespace vsgUtil
{
    class SuspendRenderPass;
    class CompileOp;
    class CompileContext;
    class UpdateThreads;
}
namespace View
{
    class LightGrid;
    class Scene;
}
namespace ESM
{
    class Cell;
    class RefNum;
}
namespace Terrain
{
    class Paging;
    class View;
}
namespace DetourNavigator
{
    struct Navigator;
    struct Settings;
}
namespace MWAnim
{
    class Object;
    class Context;
}
namespace MWRender
{
    class Scene;
    class Reflect;
    class Effects;
    class Projectiles;
    class FogManager;
    class Sky;
    class Player;
    class Pathgrid;
    class Camera;
    class Water;
    class SimpleWater;
    class TerrainStorage;
    class LandManager;
    class NavMesh;
    class ActorsPaths;
    class RecastMesh;
    class Shadow;
    class ScreenshotInterface;
    class Map;
    class WorldMap;
    class Preview;

    /*
     * Creates command graphs.
     */
    class RenderManager
    {
    public:
        RenderManager(Render::Engine& engine, vsg::ref_ptr<vsg::Node> guiRoot,
            Resource::ResourceSystem* resourceSystem /*, DetourNavigator::Navigator& navigator*/);
        ~RenderManager();

        void setStore(const MWWorld::ESMStore& store);

        RenderingInterface& getObjects();

        Resource::ResourceSystem* getResourceSystem();
        const MWAnim::Context& getAnimContext();
        double getReferenceTime() const { return 0.0; }
        Terrain::Paging* getTerrain() { return mTerrain.get(); }
        vsg::ref_ptr<vsg::OperationThreads> getOperationThreads();
        vsg::ref_ptr<vsgUtil::UpdateThreads> getUpdateThreads();
        Map* getMap() { return mMap.get(); }
        WorldMap* getWorldMap() { return mWorldMap.get(); }
        Preview* getPreview() { return mPreview.get(); }
        LandManager* getLandManager() const;
        ScreenshotInterface& getScreenshotInterface() { return *mScreenshotInterface; }

        void setNightEyeFactor(float factor);
        void setAmbientColour(const vsg::vec4& colour);
        void skySetDate(int day, int month);
        int skyGetMasserPhase() const;
        int skyGetSecundaPhase() const;
        void skySetMoonColour(bool red);

        void setSunDirection(const osg::Vec3f& direction);
        void setSunColour(const osg::Vec4f& diffuse, const osg::Vec4f& specular);

        void configureAmbient(const MWWorld::Cell& cell);
        void configureFog(const MWWorld::Cell& cell);
        void configureFog(float fogDepth, float underwaterFog, float dlFactor, float dlOffset, const vsg::vec4& colour);

        void addCell(const MWWorld::CellStore* store);
        void removeCell(const MWWorld::CellStore* store);
        void changeCellGrid(const osg::Vec3f& pos);
        void pruneCache();

        void updatePtr(const MWWorld::Ptr& old, const MWWorld::Ptr& updated);

        void rotateObject(const MWWorld::Ptr& ptr, const osg::Quat& rot);
        void moveObject(const MWWorld::Ptr& ptr, const osg::Vec3f& pos);
        void scaleObject(const MWWorld::Ptr& ptr, const osg::Vec3f& scale);

        void removeObject(const MWWorld::Ptr& ptr);

        void setWaterEnabled(bool enabled);
        void setWaterHeight(float level);

        struct RayResult
        {
            bool mHit = false;
            osg::Vec3f mHitNormalWorld;
            osg::Vec3f mHitPointWorld;
            MWWorld::Ptr mHitObject;
            ESM::RefNum mHitRefnum = { 0, -1 };
            float mRatio = 0.f;
        };

        RayResult castRay(
            const osg::Vec3f& origin, const osg::Vec3f& dest, bool ignorePlayer, bool ignoreActors = false);

        /// Return the object under the mouse cursor / crosshair position, given by nX and nY normalized screen
        /// coordinates, where (0,0) is the top left corner.
        RayResult castCameraToViewportRay(
            float nX, float nY, float maxDistance, bool ignorePlayer, bool ignoreActors = false);

        /// Get the bounding box of the given object in screen coordinates as (minX, minY, maxX, maxY), with (0,0) being
        /// the top left corner.
        osg::Vec4f getScreenBounds(const osg::BoundingBox& worldbb);

        bool toggleRenderMode(RenderMode mode);
        void setRenderModeActive(RenderMode mode, bool enabled);
        void setViewMode(ViewMode mode, bool enable);
        void setSkyEnabled(bool enable);
        Sky* getSky();

        void spawnEffect(const std::string& model, std::string_view texture, const osg::Vec3f& worldPosition,
            float scale = 1.f, bool isMagicVFX = true);

        /// Clear all savegame-specific data
        void clear();

        /// Clear all worldspace-specific data
        void notifyWorldSpaceChanged();

        void onFrame(float dt);
        void update(float dt, bool paused);

        MWAnim::Object* getObject(const MWWorld::Ptr& ptr);
        const MWAnim::Object* getObject(const MWWorld::ConstPtr& ptr) const;
        vsg::box getBoundingBox(const MWWorld::ConstPtr& ptr) const;

        void addWaterRippleEmitter(const MWWorld::Ptr& ptr);
        void removeWaterRippleEmitter(const MWWorld::Ptr& ptr);
        void emitWaterRipple(const osg::Vec3f& pos);

        void updatePlayerPtr(const MWWorld::Ptr& ptr);

        void setupPlayer(const MWWorld::Ptr& player);
        void renderPlayer(const MWWorld::Ptr& player);

        void rebuildPtr(const MWWorld::Ptr& ptr);

        void processChangedSettings(const Settings::CategorySettingVector& settings);

        float getTerrainHeightAt(const osg::Vec3f& pos);

        Camera* getCamera() { return mCamera.get(); }
        Projectiles* getProjectiles() { return mProjectiles.get(); }

        /// temporarily override the field of view with given value.
        void overrideFieldOfView(float val);
        /// reset a previous overrideFieldOfView() call, i.e. revert to field of view specified in the settings file.
        void resetFieldOfView();
        void setFieldOfView(float val);
        float getFieldOfView() const;
        float getViewDistance() const { return mViewDistance; }
        void setViewDistance(float distance, bool dummy = true);

        osg::Vec3f getHalfExtents(const MWWorld::ConstPtr& object) const;

        void exportSceneGraph(const MWWorld::Ptr& ptr, const std::filesystem::path& dir);

        bool toggleBorders();

        void updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
            const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end) const;

        void removeActorPath(const MWWorld::ConstPtr& actor) const;

        void setNavMeshNumber(const std::size_t value);

        bool pagingEnableObject(int type, const MWWorld::ConstPtr& ptr, bool enabled);
        void pagingBlacklistObject(int type, const MWWorld::ConstPtr& ptr);
        bool pagingUnlockCache();
        void getPagedRefnums(const osg::Vec4i& activeGrid, std::set<ESM::RefNum>& out);

        void updateExtents(uint32_t w, uint32_t h);

        vsg::dmat4 getProjectionMatrix() const;

    private:
        vsg::ref_ptr<vsg::Node> createMapScene();
        vsg::Group::Children createCastShadowScene(bool exterior);
        void ensureCompiled();
        void createIntersectorScene();
        void updateSceneChildren();
        void updateSceneData();
        void updateProjectionMatrix();
        void setupScreenshotInterface();
        void setFirstPersonMode(bool enabled);
        void updateTerrain(const vsg::dvec3& pos);

        struct Mode
        {
            bool active = true;
            bool toggled = true;
            operator bool() const { return active && toggled; }
        };
        std::vector<Mode> mRenderModes;

        Render::Engine& mEngine;
        vsg::ref_ptr<vsg::StateGroup> mSceneRoot;
        vsg::ref_ptr<vsg::Group> mIntersectorScene;
        vsg::ref_ptr<vsg::Switch> mPlayerGroup;
        vsg::ref_ptr<vsg::View> mView;
        std::unique_ptr<Shadow> mShadow;
        std::unique_ptr<View::LightGrid> mLightGrid;
        std::unique_ptr<View::Scene> mSceneData;
        vsg::ref_ptr<vsg::Node> mGui;
        vsg::ref_ptr<vsg::Camera> mSceneCamera;
        vsg::ref_ptr<vsg::LookAt> mViewMatrix;
        vsg::ref_ptr<vsg::Perspective> mProjectionMatrix;
        vsg::ref_ptr<vsg::Group> mFirstPersonProjection;

        vsg::ref_ptr<vsg::RenderGraph> mRenderGraph;
        vsg::ref_ptr<vsg::RenderGraph> mGuiRenderGraph;
        vsg::ref_ptr<vsg::CommandGraph> mCommandGraph;
        vsg::ref_ptr<vsg::RecordTraversal> mRecord;
        std::vector<vsg::ref_ptr<vsg::RecordTraversal>> mRecordShadow;
        vsg::ref_ptr<vsgUtil::CompileContext> mCompileContext;
        vsg::ref_ptr<vsg::OperationThreads> mOperationThreads;
        vsg::ref_ptr<vsgUtil::CompileOp> mCompileOp;

        Resource::ResourceSystem* mResourceSystem;

        std::unique_ptr<Reflect> mReflect;
        std::unique_ptr<Scene> mScene;
        std::unique_ptr<Water> mWater;
        std::unique_ptr<SimpleWater> mSimpleWater;
        std::unique_ptr<Terrain::Paging> mTerrain;
        std::unique_ptr<TerrainStorage> mTerrainStorage;
        std::unique_ptr<Terrain::View> mTerrainView;
        std::unique_ptr<Sky> mSky;
        std::unique_ptr<FogManager> mFog;
        std::unique_ptr<Effects> mEffects;
        std::unique_ptr<Projectiles> mProjectiles;
        std::unique_ptr<Player> mPlayer;
        std::unique_ptr<Camera> mCamera;
        // std::unique_ptr<Groundcover> mGroundcover;
        // DetourNavigator::Navigator& mNavigator;
        vsg::ref_ptr<vsgUtil::SuspendRenderPass> mSuspend;
        vsg::ref_ptr<vsg::Switch> mScreenshotSwitch;
        std::unique_ptr<ScreenshotInterface> mScreenshotInterface;
        std::unique_ptr<Preview> mPreview;
        std::unique_ptr<WorldMap> mWorldMap;
        std::unique_ptr<Map> mMap;

        vsg::vec4 mAmbientColor;
        vsg::vec4 mLightDiffuse;
        vsg::vec4 mLightSpecular;
        vsg::vec3 mLightPosition;
        float mMinimumAmbientLuminance;
        float mNightEyeFactor = 0;

        float mNearClip;
        float mViewDistance;
        std::optional<float> mFieldOfViewOverride = 0;
        float mFieldOfView;

        std::size_t mNavMeshNumber = 0;
        // std::unique_ptr<ActorsPaths> mActorsPaths;
        // std::unique_ptr<NavMesh> mNavMesh;
        // std::unique_ptr<RecastMesh> mRecastMesh;
        // std::unique_ptr<Pathgrid> mPathgrid;

        void operator=(const RenderManager&);
        RenderManager(const RenderManager&);
    };
}

#endif
