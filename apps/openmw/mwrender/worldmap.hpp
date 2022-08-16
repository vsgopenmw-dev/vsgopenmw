#ifndef GAME_RENDER_GLOBALMAP_H
#define GAME_RENDER_GLOBALMAP_H

#include <string>
#include <vector>
#include <map>

#include <vsg/core/Array2D.h>

namespace vsg
{
    class ImageView;
    class Context;
    class Node;
    class OperationThreads;
}
namespace ESM
{
    struct GlobalMap;
}
namespace MWRender
{
    class WorldMap
    {
    public:
        WorldMap(vsg::Context &ctx, const vsg::Options *shaderOptions, int cellSize);
        ~WorldMap();

        int getWidth() const { return mWidth; }
        int getHeight() const { return mHeight; }

        void worldPosToImageSpace(float x, float z, float& imageX, float& imageY);

        void exploreCell (int cellX, int cellY, vsg::ImageView *localMapTexture);

        /// Clears the overlay
        void clear();

        void write (ESM::GlobalMap& map);
        void read (ESM::GlobalMap& map);

        vsg::ref_ptr<vsg::Data> getBaseTexture();
        vsg::ImageView *getOverlayTexture();

        void ensureLoaded();

        void asyncSave();
        void finishThreads();

        vsg::Node *node();

    private:
        class UpdateOverlay;
        vsg::ref_ptr<UpdateOverlay> mUpdate;

        const int mCellSize;

        std::vector<std::pair<int,int>> mExploredCells;

        vsg::ref_ptr<vsg::ubvec3Array2D> mBaseData;
        vsg::ref_ptr<vsg::ImageView> mAlphaTexture;

        // GPU copy of overlay
        vsg::ref_ptr<vsg::ImageView> mOverlayTexture;

        // CPU copy of overlay
        vsg::ref_ptr<vsg::ubvec4Array2D> mOverlayData;

        vsg::ref_ptr<vsg::OperationThreads> mOperationThreads;

        class CreateMap;
        vsg::ref_ptr<CreateMap> mCreateMap;
        class Save;
        vsg::ref_ptr<Save> mSave;

        uint32_t mWidth;
        uint32_t mHeight;
        int mMinX = 0, mMaxX = 0, mMinY = 0, mMaxY = 0;

        vsg::ref_ptr<vsg::Context> mContext;
        vsg::ref_ptr<const vsg::Options> mShaderOptions;
    };
}

#endif
