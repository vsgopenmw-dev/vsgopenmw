#ifndef VSGOPENMW_MWRENDER_MAP_H
#define VSGOPENMW_MWRENDER_MAP_H

#include <vsg/core/ref_ptr.h>
#include <vsg/maths/box.h>
#include <vsg/maths/quat.h>
#include <vsg/vk/vulkan.h>

#include <map>
#include <set>
#include <memory>
#include <vector>

namespace MWWorld
{
    class CellStore;
}
namespace ESM
{
    struct FogTexture;
}
namespace Terrain
{
    class Paging;
}
namespace vsg
{
    class Node;
    class Data;
    class Context;
    class ImageView;
    class Options;
}
namespace vsgUtil
{
    class CompileContext;
}
namespace MWRender
{
    /// \brief Local map rendering
    class Map
    {
    public:
        Map(vsg::Context& ctx, VkSampleCountFlagBits samples, vsg::ref_ptr<vsgUtil::CompileContext> compile, vsg::ref_ptr<vsg::Node> scene, Terrain::Paging* terrain, float referenceViewDistance, uint32_t resolution, const vsg::Options* shaderOptions);
        ~Map();

        /**
         * Clear all savegame-specific data (i.e. fog of war textures)
         */
        void clear();

        /**
         * Request a map render for the given cell. Render textures can be retrieved with the getMapTexture function.
         */
        void requestExteriorMap(const MWWorld::CellStore* cell);
        void requestInteriorMap(const MWWorld::CellStore* cell);

        void addCell(MWWorld::CellStore* cell);
        void removeCell(int x, int y);
        void removeCell(MWWorld::CellStore* cell);

        vsg::ImageView* getMapTexture(int x, int y);

        vsg::ImageView* getFogOfWarTexture(int x, int y);

        /**
         * Set the position & direction of the player, and returns the position in map space through the reference
         * parameters.
         * @remarks This is used to draw a "fog of war" effect
         * to hide areas on the map the player has not discovered yet.
         */
        void updatePlayer(const vsg::vec3& position, const vsg::quat& orientation, float& u, float& v, int& x, int& y,
            vsg::vec3& direction);

        /**
         * Save the fog of war for this cell to its CellStore.
         * @remarks This should be called when unloading a cell, and for all active cells prior to saving the game.
         */
        void saveFogOfWar(MWWorld::CellStore* cell);

        /**
         * Get the interior map texture index and normalized position on this texture, given a world position
         */
        void worldToInteriorMapPosition(vsg::dvec2 pos, float& nX, float& nY, int& x, int& y);

        vsg::dvec2 interiorMapToWorldPosition(float nX, float nY, int x, int y);

        /**
         * Check if a given position is explored by the player (i.e. not obscured by fog of war)
         */
        bool isPositionExplored(float nX, float nY, int x, int y) const;

        /*
         * Check if a given cell has existing fog state in flight.
         */
        bool isCellExplored(int x, int y) const;

        vsg::Node* getNode();
        vsg::Node* getFogNode();

    private:
        vsg::ref_ptr<vsg::Context> mContext;

        vsg::ref_ptr<vsg::Node> mSceneGraph;
        vsg::ref_ptr<vsg::Data> mLightData;

        Terrain::Paging* mTerrain;
        float mReferenceViewDistance;

        class Blit;
        vsg::ref_ptr<Blit> mBlit;
        class UpdateFog;
        vsg::ref_ptr<UpdateFog> mUpdateFog;

        class Texture;
        class FogTexture;
        std::vector<std::unique_ptr<Texture>> mTextures;
        std::vector<std::unique_ptr<FogTexture>> mFogTextures;
        std::vector<Texture*> mAvailableTextures;
        std::vector<FogTexture*> mAvailableFogTextures;

        struct ViewData;
        std::vector<std::unique_ptr<ViewData>> mViewData;

        struct Segment
        {
            void clearFogOfWar();
            bool loadFogOfWar(const ESM::FogTexture& fog);
            void saveFogOfWar(ESM::FogTexture& fog) const;

            Texture* mapTexture{};
            FogTexture* fogTexture{};
            const MWWorld::CellStore* cell{};
            std::set<std::pair<int, int>> renderedGrid;

            bool needUpdate = true;
            bool hasFogState = false;
        };

        using SegmentMap = std::map<std::pair<int, int>, Segment>;
        SegmentMap mSegments;

        // size of a map segment (for exteriors, 1 cell)
        float mMapWorldSize;

        int mCellDistance;
        uint32_t mResolution;
        float mAngle{};

        bool mInterior{};
        vsg::dbox mBounds;

        vsg::ref_ptr<vsgUtil::CompileContext> mCompile;

        ViewData* setupRenderToTexture(
            int segment_x, int segment_y, float left, float top, const vsg::dvec3& upVector, float zmin, float zmax);
        Texture* getOrCreateTexture();
        FogTexture* getOrCreateFogTexture();
        ViewData* getOrCreateViewData();
        void clearSegments();
        void recycle(Segment&);
        void loadFogOfWar(Segment& segment);
    };

}
#endif
