#ifndef VSGOPENMW_MWRENDER_MAP_H
#define VSGOPENMW_MWRENDER_MAP_H

#include <vsg/core/ref_ptr.h>
#include <vsg/maths/box.h>
#include <vsg/maths/quat.h>

#include <set>
#include <vector>
#include <map>
#include <memory>

namespace MWWorld
{
    class CellStore;
}
namespace ESM
{
    struct FogTexture;
}
namespace vsg
{
    class Node;
    class Context;
    class ImageInfo;
    class ImageView;
    class Options;
}
namespace MWRender
{
    /// \brief Local map rendering
    class Map
    {
    public:
        Map(vsg::Context &ctx, vsg::ref_ptr<vsg::Node> scene, uint32_t resolution, const vsg::Options *shaderOptions);
        ~Map();

        /**
         * Clear all savegame-specific data (i.e. fog of war textures)
         */
        void clear();

        /**
         * Request a map render for the given cell. Render textures can be retrieved with the getMapTexture function.
         */
        void requestExteriorMap(const MWWorld::CellStore* cell);
        void requestInteriorMap(const MWWorld::CellStore* cell, const vsg::vec2 &north);

        void addCell(MWWorld::CellStore* cell);
        void removeExteriorCell(int x, int y);

        void removeCell (MWWorld::CellStore* cell);

        vsg::ImageView *getMapTexture (int x, int y);

        vsg::ImageView *getFogOfWarTexture (int x, int y);

        /**
         * Set the position & direction of the player, and returns the position in map space through the reference parameters.
         * @remarks This is used to draw a "fog of war" effect
         * to hide areas on the map the player has not discovered yet.
         */
        void updatePlayer (const vsg::vec3 &position, const vsg::quat &orientation, float& u, float& v, int& x, int& y, vsg::vec3 &direction);

        /**
         * Save the fog of war for this cell to its CellStore.
         * @remarks This should be called when unloading a cell, and for all active cells prior to saving the game.
         */
        void saveFogOfWar(MWWorld::CellStore* cell);

        /**
         * Get the interior map texture index and normalized position on this texture, given a world position
         */
        void worldToInteriorMapPosition (vsg::dvec2 pos, float& nX, float& nY, int& x, int& y);

        vsg::dvec2 interiorMapToWorldPosition (float nX, float nY, int x, int y);

        /**
         * Check if a given position is explored by the player (i.e. not obscured by fog of war)
         */
        bool isPositionExplored (float nX, float nY, int x, int y);

        vsg::Node *getNode();
        vsg::Node *getFogNode();

        const vsg::ref_ptr<vsg::Context> context;
    private:
        vsg::ref_ptr<vsg::Node> mScene;

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

        using Grid = std::set<std::pair<int, int>>;
        Grid mCurrentGrid;

        struct Segment
        {
            void createFogData();
            void initFogOfWar();
            bool loadFogOfWar(const ESM::FogTexture& fog, vsg::Context &ctx);
            void saveFogOfWar(ESM::FogTexture& fog) const;

            Texture *mapTexture{};
            FogTexture *fogTexture{};

            bool needUpdate = true;
            bool hasFogState = false;
        };

        using SegmentMap = std::map<std::pair<int, int>, Segment>;
        SegmentMap mExteriorSegments;
        SegmentMap mInteriorSegments;

        // size of a map segment (for exteriors, 1 cell)
        float mMapWorldSize;

        int mCellDistance;
        uint32_t mResolution;

        float mAngle{};
        void setupRenderToTexture(int segment_x, int segment_y, float left, float top, const vsg::dvec3 &upVector, float zmin, float zmax);
        Texture *getOrCreateTexture();
        FogTexture *getOrCreateFogTexture();
        void clearInteriorSegments();
        void recycle(Segment &);

        bool mInterior{};
        vsg::dbox mBounds;
    };

}
#endif
