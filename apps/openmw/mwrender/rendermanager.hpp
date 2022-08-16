#ifndef VSGOPENMW_MWRENDER_RENDERMANAGER_H
#define VSGOPENMW_MWRENDER_RENDERMANAGER_H

#include <components/settings/settings.hpp>

#include "renderinginterface.hpp"
#include "rendermode.hpp"

#include <deque>
#include <memory>
#include <optional>

#include <vsg/core/ref_ptr.h>
#include <vsg/maths/box.h>

#include <osg/ref_ptr>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osg/Vec4i>
#include <osg/Quat>
#include <osg/BoundingBox>

namespace MWWorld
{
    class ESMStore;
    class Ptr;
    class ConstPtr;
    class CellStore;
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
    class Node;
    class Context;
    class Camera;
    class LookAt;
    class Perspective;
    class View;
}
namespace View
{
    class LightGrid;
    class Descriptors;
}
namespace ESM
{
    class Cell;
    class RefNum;
}
namespace Terrain
{
    class Group;
}
namespace DetourNavigator
{
    struct Navigator;
    struct Settings;
}
namespace MWRender
{
    class Effects;
    class Projectiles;
    class ScreenshotManager;
    class FogManager;
    class Sky;
    class Player;
    class Pathgrid;
    class Camera;
    class Water;
    class TerrainStorage;
    class LandManager;
    class NavMesh;
    class ActorsPaths;
    class RecastMesh;
    class Shadow;

    /*
     * Creates command graphs.
     */
    class RenderManager : public MWRender::RenderingInterface
    {
    public:
        RenderManager(Render::Engine &engine, vsg::Context *context, vsg::Node *guiRoot, vsg::Node *guiRenderTextures, Resource::ResourceSystem* resourceSystem, const std::string& resourcePath/*, DetourNavigator::Navigator& navigator*/);
        ~RenderManager();

        Scene& getObjects() override;
        vsg::ref_ptr<vsg::Node> createMapScene();

        Resource::ResourceSystem* getResourceSystem();

        //Terrain::World* getTerrain();

        double getReferenceTime() const;

        void setNightEyeFactor(float factor);

        void setAmbientColour(const vsg::vec4 &colour);

        void skySetDate(int day, int month);
        int skyGetMasserPhase() const;
        int skyGetSecundaPhase() const;
        void skySetMoonColour(bool red);

        void setSunDirection(const osg::Vec3f& direction);
        void setSunColour(const osg::Vec4f& diffuse, const osg::Vec4f& specular);

        void configureAmbient(const ESM::Cell* cell);
        void configureFog(const ESM::Cell* cell);
        void configureFog(float fogDepth, float underwaterFog, float dlFactor, float dlOffset, const vsg::vec4& colour);

        void addCell(const MWWorld::CellStore* store);
        void removeCell(const MWWorld::CellStore* store);

        void enableTerrain(bool enable);

        void updatePtr(const MWWorld::Ptr& old, const MWWorld::Ptr& updated);

        void rotateObject(const MWWorld::Ptr& ptr, const osg::Quat& rot);
        void moveObject(const MWWorld::Ptr& ptr, const osg::Vec3f& pos);
        void scaleObject(const MWWorld::Ptr& ptr, const osg::Vec3f& scale);

        void removeObject(const MWWorld::Ptr& ptr);

        void setWaterEnabled(bool enabled);
        void setWaterHeight(float level);

        /// Take a screenshot of w*h onto the given image, not including the GUI.
        //void screenshot(osg::Image* image, int w, int h);

        struct RayResult
        {
            bool mHit = false;
            osg::Vec3f mHitNormalWorld;
            osg::Vec3f mHitPointWorld;
            MWWorld::Ptr mHitObject;
            ESM::RefNum mHitRefnum = {0,-1};
            float mRatio = 0.f;
        };

        RayResult castRay(const osg::Vec3f& origin, const osg::Vec3f& dest, bool ignorePlayer, bool ignoreActors=false);

        /// Return the object under the mouse cursor / crosshair position, given by nX and nY normalized screen coordinates,
        /// where (0,0) is the top left corner.
        RayResult castCameraToViewportRay(float nX, float nY, float maxDistance, bool ignorePlayer, bool ignoreActors=false);

        /// Get the bounding box of the given object in screen coordinates as (minX, minY, maxX, maxY), with (0,0) being the top left corner.
        osg::Vec4f getScreenBounds(const osg::BoundingBox &worldbb);

        bool toggleRenderMode(RenderMode mode);
        void setRenderMode(RenderMode mode, bool enabled);
        void setViewMode(ViewMode mode, bool enable);
        void setSkyEnabled(bool enable);
        Sky* getSky();

        void spawnEffect(const std::string &model, const std::string &texture, const osg::Vec3f &worldPosition, float scale = 1.f, bool isMagicVFX = true);

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

        void updatePlayerPtr(const MWWorld::Ptr &ptr);

        void removePlayer(const MWWorld::Ptr& player);
        void setupPlayer(const MWWorld::Ptr& player);
        void renderPlayer(const MWWorld::Ptr& player);

        void rebuildPtr(const MWWorld::Ptr& ptr);

        void processChangedSettings(const Settings::CategorySettingVector& settings);

        float getTerrainHeightAt(const osg::Vec3f& pos);

        Camera *getCamera() { return mCamera.get(); }
        Projectiles *getProjectiles() { return mProjectiles.get(); }

        /// temporarily override the field of view with given value.
        void overrideFieldOfView(float val);
        /// reset a previous overrideFieldOfView() call, i.e. revert to field of view specified in the settings file.
        void resetFieldOfView();
        void setFieldOfView(float val);
        float getFieldOfView() const;
        float getViewDistance() const { return mViewDistance; }
        void setViewDistance(float distance, bool dummy=true);

        osg::Vec3f getHalfExtents(const MWWorld::ConstPtr& object) const;

        void exportSceneGraph(const MWWorld::Ptr& ptr, const std::string& folder);

        LandManager* getLandManager() const;

        bool toggleBorders();

        void updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
                const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end) const;

        void removeActorPath(const MWWorld::ConstPtr& actor) const;

        void setNavMeshNumber(const std::size_t value);

        void setActiveGrid(const osg::Vec4i &grid);

        bool pagingEnableObject(int type, const MWWorld::ConstPtr& ptr, bool enabled);
        void pagingBlacklistObject(int type, const MWWorld::ConstPtr &ptr);
        bool pagingUnlockCache();
        void getPagedRefnums(const osg::Vec4i &activeGrid, std::set<ESM::RefNum> &out);

        void setCompileRequired(bool);
        void updateExtents(uint32_t w, uint32_t h);

    private:
        vsg::ref_ptr<vsg::Node> createCastShadowScene(bool exterior);
        void createIntersectorScene();
        void updateSceneChildren();
        void updateSceneData(float dt);
        void updateProjectionMatrix();

        void updateNavMesh();

        void updateRecastMesh();
        bool mUseTerrain{};
        bool mUseSky{};
        std::vector<bool> mRenderModes;

        Render::Engine &mEngine;
        vsg::ref_ptr<vsg::StateGroup> mSceneRoot;
        vsg::ref_ptr<vsg::Group> mIntersectorScene;
        vsg::ref_ptr<vsg::Switch> mPlayerGroup;
        vsg::ref_ptr<vsg::View> mView;
        std::unique_ptr<Shadow> mShadow;
        std::unique_ptr<View::LightGrid> mLightGrid;
        std::unique_ptr<View::Descriptors> mDescriptors;
        vsg::ref_ptr<vsg::Node> mGui;
        vsg::ref_ptr<vsg::Camera> mSceneCamera;
        vsg::ref_ptr<vsg::LookAt> mViewMatrix;
        vsg::ref_ptr<vsg::Perspective> mProjectionMatrix;

        vsg::ref_ptr<vsg::RenderGraph> mRenderGraph;
        vsg::ref_ptr<vsg::CommandGraph> mCommandGraph;
        vsg::ref_ptr<vsg::RecordTraversal> mRecordScene;
        vsg::ref_ptr<vsg::RecordTraversal> mRecord;
        std::vector<vsg::ref_ptr<vsg::RecordTraversal>> mRecordShadow;
        vsg::ref_ptr<vsg::CompileManager> mCompileManager;

        Resource::ResourceSystem* mResourceSystem;

        //DetourNavigator::Navigator& mNavigator;
        std::unique_ptr<Scene> mScene;
        //std::unique_ptr<Water> mWater;
        std::unique_ptr<Terrain::Group> mTerrain;
        std::unique_ptr<TerrainStorage> mTerrainStorage;
        //std::unique_ptr<Groundcover> mGroundcover;
        std::unique_ptr<Sky> mSky;
        std::unique_ptr<FogManager> mFog;
        //std::unique_ptr<ScreenshotManager> mScreenshotManager;
        std::unique_ptr<Effects> mEffects;
        std::unique_ptr<Projectiles> mProjectiles;
        std::unique_ptr<Player> mPlayer;
        std::unique_ptr<Camera> mCamera;

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
        float mFirstPersonFieldOfView;

        std::size_t mNavMeshNumber = 0;
        //std::unique_ptr<ActorsPaths> mActorsPaths;
        //std::unique_ptr<NavMesh> mNavMesh;
        //std::unique_ptr<RecastMesh> mRecastMesh;
        //std::unique_ptr<Pathgrid> mPathgrid;

        void operator = (const RenderManager&);
        RenderManager(const RenderManager&);
    };
}

#endif
