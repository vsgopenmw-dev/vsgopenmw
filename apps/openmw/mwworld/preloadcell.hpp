#ifndef VSGOPENMW_MWWORLD_PRELOADCELL_H
#define VSGOPENMW_MWWORLD_PRELOADCELL_H

#include <map>
#include <chrono>

#include <vsg/core/ref_ptr.h>

namespace vsg
{
    class OperationThreads;
    class Options;
}
namespace MWRender
{
    class LandManager;
}
namespace MWAnim
{
    class Context;
}
namespace MWWorld
{
    class CellStore;

    class PreloadCell;
    /*
     * Asynchronously prepares requested grid cells.
     */
    class CellPreloader
    {
    public:
        CellPreloader(const MWAnim::Context& animContext, vsg::ref_ptr<const vsg::Options> actorShapeOptions,
            vsg::ref_ptr<const vsg::Options> shapeOptions,
            MWRender::LandManager* landManager, vsg::ref_ptr<vsg::OperationThreads> threads);
        ~CellPreloader();
        vsg::ref_ptr<vsg::OperationThreads> getCompileThreads();

        /// Ask a background thread to preload rendering meshes and collision shapes for objects in this cell.
        /// @note The cell itself must be in State_Loaded or State_Preloaded.
        float /*complete*/ preload(MWWorld::CellStore& cell);

        void clear();

        /// Removes preloaded cells that have not had a preload request for a while.
        void updateCache();

        /// How long to keep a preloaded cell in cache after it's no longer requested.
        void setExpiryDelay(double expiryDelay);

        /// The minimum number of preloaded cells before unused cells get thrown out.
        void setMinCacheSize(unsigned int num);

        /// The maximum number of preloaded cells.
        void setMaxCacheSize(unsigned int num);

        unsigned int getMaxCacheSize() const;

    private:
        const MWAnim::Context& mAnimContext;
        vsg::ref_ptr<const vsg::Options> mActorShapeOptions;
        vsg::ref_ptr<const vsg::Options> mShapeOptions;
        MWRender::LandManager* mLandManager;
        vsg::ref_ptr<vsg::OperationThreads> mThreads;
        vsg::ref_ptr<vsg::OperationThreads> mCompileThreads;
        double mExpiryDelay = 1.0;
        unsigned int mMinCacheSize{};
        unsigned int mMaxCacheSize{};

        struct PreloadEntry
        {
            std::chrono::steady_clock::time_point timeStamp;
            vsg::ref_ptr<PreloadCell> operation;
        };

        // Cells that are currently being preloaded, or have already finished preloading
        std::map<const MWWorld::CellStore*, PreloadEntry> mPreloadCells;

        vsg::ref_ptr<vsg::OperationThreads> mOperationThreads;
    };

}

#endif
