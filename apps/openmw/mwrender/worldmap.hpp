#ifndef VSGOPENMW_MWRENDER_WORLDMAP_H
#define VSGOPENMW_MWRENDER_WORLDMAP_H

#include <map>
#include <string>
#include <vector>
#include <limits>

#include <vsg/core/Array2D.h>

namespace vsg
{
    class ImageView;
    class Node;
    class OperationThreads;
}
namespace vsgUtil
{
    class CompileContext;
}
namespace ESM
{
    struct GlobalMap;
}
namespace MWWorld
{
    class ESMStore;
}
namespace MWRender
{
    class WorldMap
    {
    public:
        WorldMap(vsg::ref_ptr<vsgUtil::CompileContext> compile, const vsg::Options* shaderOptions, vsg::ref_ptr<vsg::OperationThreads> threads);
        ~WorldMap();

        void load(const MWWorld::ESMStore& esmStore, int cellSize);

        int getWidth() const { return mWidth; }
        int getHeight() const { return mHeight; }

        void worldPosToImageSpace(float x, float y, float& imageX, float& imageY) const;

        void exploreCell(int cellX, int cellY, vsg::ImageView* localMapTexture);

        /// Clears the overlay
        void clear();

        void write(ESM::GlobalMap& map);
        void read(ESM::GlobalMap& map);

        vsg::ref_ptr<vsg::Data> getBaseTexture();
        vsg::ImageView* getOverlayTexture();

        void ensureLoaded();

        void asyncSave();
        void finishThreads();

        vsg::Node* node();

    private:
        class UpdateOverlay;
        vsg::ref_ptr<UpdateOverlay> mUpdate;

        int mCellSize{};

        vsg::ref_ptr<vsg::ubvec3Array2D> mBaseData;
        vsg::ref_ptr<vsg::ImageView> mAlphaTexture;

        // GPU copy of overlay
        vsg::ref_ptr<vsg::ImageView> mOverlayTexture;

        vsg::ref_ptr<vsg::OperationThreads> mOperationThreads;

        class CreateMap;
        vsg::ref_ptr<CreateMap> mCreateMap;
        class Save;
        vsg::ref_ptr<Save> mSave;

        vsg::ref_ptr<vsgUtil::CompileContext> mCompile;
        vsg::ref_ptr<const vsg::Options> mShaderOptions;

        uint32_t mWidth{};
        uint32_t mHeight{};

        struct Bounds
        {
            int minX = std::numeric_limits<int>::max();
            int maxX = std::numeric_limits<int>::min();
            int minY = std::numeric_limits<int>::max();
            int maxY = std::numeric_limits<int>::min();
            bool valid() const;
            void addCellArea(int cellX, int cellY);
            static Bounds intersect(const Bounds& lhs, const Bounds& rhs);
            static Bounds read(const ESM::GlobalMap& esm);
            void write(ESM::GlobalMap& esm) const;
        };
        Bounds mBounds;
        Bounds mExploredBounds;

        vsg::ivec4 cropImageRect(const Bounds& srcBounds, const Bounds& dstBounds, int cellWidth, int cellHeight, int imageWidth, int imageHeight) const;
    };
}

#endif
