#include "preloadcell.hpp"

#include <atomic>
#include <limits>
#include <iostream>

#include <vsg/core/Objects.h>
#include <vsg/io/read.h>
#include <vsg/threading/OperationThreads.h>

#include <components/esm3/loadcell.hpp>
#include <components/mwanimation/context.hpp>
#include <components/animation/controllermap.hpp>
#include <components/vsgutil/compilecontext.hpp>
#include <components/vsgutil/compileop.hpp>

#include "../mwrender/landmanager.hpp"

#include "cellstore.hpp"
#include "class.hpp"

namespace MWWorld
{

    /// Worker thread item: preload models in a cell.
    class PreloadCell : public vsgUtil::Operation
    {
        bool mIsExterior;
        int mX;
        int mY;
        std::atomic_int mProgress = 0;
        int mProgressRange = 1;
        std::vector<std::string> mMeshes;
        std::vector<bool> mUseAnim;
        const MWAnim::Context& mAnimContext;
        vsg::ref_ptr<const vsg::Options> mActorShapeOptions;
        vsg::ref_ptr<const vsg::Options> mShapeOptions;
        MWRender::LandManager* mLandManager;

        // keep a ref to the loaded objects to make sure it stays loaded as long as this cell is in the preloaded state
        std::set<vsg::ref_ptr<const vsg::Object>> mPreloadedObjects;
        vsg::ref_ptr<vsg::Objects> mPreloadedNodes;

    public:
        vsg::ref_ptr<vsg::OperationThreads> compileThreads;

        /// Constructor to be called from the main thread.
        PreloadCell(MWWorld::CellStore& cell, const MWAnim::Context& animContext, vsg::ref_ptr<const vsg::Options> actorShapeOptions,
            vsg::ref_ptr<const vsg::Options> shapeOptions,
            MWRender::LandManager* landManager)
            : mIsExterior(cell.getCell()->isExterior())
            , mX(cell.getCell()->getGridX())
            , mY(cell.getCell()->getGridY())
            , mAnimContext(animContext)
            , mActorShapeOptions(actorShapeOptions)
            , mShapeOptions(shapeOptions)
            , mLandManager(landManager)
        {
            mPreloadedNodes = vsg::Objects::create();
            cell.forEach(*this);
            mProgressRange = mMeshes.size();
            if (mIsExterior)
                ++mProgressRange;
        }
        bool operator()(const MWWorld::Ptr& ptr)
        {
            ptr.getClass().getModelsToPreload(ptr, mMeshes);
            mUseAnim.resize(mMeshes.size(), ptr.getClass().useAnim());
            return true;
        }

        std::atomic_bool abort = false;

        /// Preload work to be called from the worker thread.
        void operate() override
        {
            if (mIsExterior)
            {
                try
                {
                    mPreloadedObjects.insert(mLandManager->getLand({ mX, mY, {} }));
                }
                catch (std::exception&) {}
                ++mProgress;
            }
            for (size_t i = 0; !abort && i < mMeshes.size(); ++i)
            {
                const auto& mesh = mMeshes[i];
                bool useAnim = mUseAnim[i];
                try
                {
                    if (useAnim)
                    {
                        if (auto anim = mAnimContext.readAnimation(mesh))
                            mPreloadedObjects.insert(anim);
                    }
                    auto& nodeOptions = useAnim ? mAnimContext.actorOptions : mAnimContext.nodeOptions;
                    if (auto node = vsg::read(mesh, nodeOptions))
                    {
                        if (mPreloadedObjects.insert(node).second)
                            mPreloadedNodes->children.push_back(node);
                    }
                    auto& shapeOptions = useAnim ? mActorShapeOptions : mShapeOptions;
                    if (auto shape = vsg::read(mesh, shapeOptions))
                        mPreloadedObjects.insert(shape);
                }
                catch (std::exception&)
                {
                    // ignore error for now, would spam the log too much
                    // error will be shown when visiting the cell
                }
                ++mProgress;
            }
            if (!mPreloadedNodes->children.empty())
            {
                if (compileThreads)
                    compileThreads->add(vsg::ref_ptr{ new vsgUtil::CompileOp(mPreloadedNodes, mAnimContext.compileContext) });
                else
                    mAnimContext.compileContext->compile(mPreloadedNodes);
            }

            try
            {
                // keep these always loaded just in case
                mPreloadedObjects.insert(mAnimContext.readActor(mAnimContext.files.baseanim));
                mPreloadedObjects.insert(mAnimContext.readActor(mAnimContext.files.baseanimkna));
                mPreloadedObjects.insert(mAnimContext.readActor(mAnimContext.files.baseanimfemale));
            }
            catch (std::exception&) {}

            mProgress = mProgressRange;
        }

        float getComplete() const
        {
            if (mProgress == mProgressRange)
                return 1;
            return static_cast<float>(mProgress)/static_cast<float>(mProgressRange);
        }
    };

    CellPreloader::CellPreloader(const MWAnim::Context& animContext, vsg::ref_ptr<const vsg::Options> actorShapeOptions,
        vsg::ref_ptr<const vsg::Options> shapeOptions,
        MWRender::LandManager* landManager, vsg::ref_ptr<vsg::OperationThreads> threads)
        : mAnimContext(animContext)
        , mActorShapeOptions(actorShapeOptions)
        , mShapeOptions(shapeOptions)
        , mLandManager(landManager)
        , mCompileThreads(vsg::OperationThreads::create(1/*CompileManager::numCompileTraversals*/)) //if(supportsNonBlockingCompile) mCompileThreads={}
        , mOperationThreads(threads)
    {
    }

    CellPreloader::~CellPreloader()
    {
        for (auto& [cell, item] : mPreloadCells)
            item.operation->abort = true;
        for (auto& [cell, item] : mPreloadCells)
            item.operation->wait();
        mPreloadCells.clear();
    }

    float CellPreloader::preload(CellStore& cell)
    {
        if (cell.getState() == CellStore::State_Unloaded)
        {
            std::cerr << "Error: can't preload objects for unloaded cell" << std::endl;
            return 1;
        }

        auto timestamp = std::chrono::steady_clock::now();
        auto found = mPreloadCells.find(&cell);
        if (found != mPreloadCells.end())
        {
            // already preloaded, nothing to do other than updating the timestamp
            found->second.timeStamp = timestamp;
            return found->second.operation->getComplete();
        }

        while (mMaxCacheSize > 0 && mPreloadCells.size() >= mMaxCacheSize)
        {
            // throw out oldest cell to make room
            auto oldestCell = mPreloadCells.begin();
            std::optional<std::chrono::steady_clock::time_point> oldestTimestamp;
            double threshold = 1.0; // seconds
            for (auto it = mPreloadCells.begin(); it != mPreloadCells.end(); ++it)
            {
                if (!oldestTimestamp || it->second.timeStamp < *oldestTimestamp)
                {
                    oldestTimestamp = it->second.timeStamp;
                    oldestCell = it;
                }
            }

            if (std::chrono::duration_cast<std::chrono::duration<double>>(timestamp - *oldestTimestamp).count() > threshold)
            {
                oldestCell->second.operation->abort = true;
                mPreloadCells.erase(oldestCell);
            }
            else
                return 1;
        }

        vsg::ref_ptr<PreloadCell> item(new PreloadCell(cell, mAnimContext, mActorShapeOptions, mShapeOptions, mLandManager));
        item->compileThreads = mCompileThreads;
        mOperationThreads->add(item);

        mPreloadCells[&cell] = PreloadEntry{timestamp, item};
        return 0;
    }

    void CellPreloader::updateCache()
    {
        auto timestamp = std::chrono::steady_clock::now();
        for (auto it = mPreloadCells.begin(); it != mPreloadCells.end();)
        {
            if (mPreloadCells.size() >= mMinCacheSize && std::chrono::duration_cast<std::chrono::duration<double>>(timestamp - it->second.timeStamp).count() > mExpiryDelay)
            {
                if (it->second.operation)
                    it->second.operation->abort = true;
                mPreloadCells.erase(it++);
            }
            else
                ++it;
        }
    }

    void CellPreloader::clear()
    {
        for (auto it = mPreloadCells.begin(); it != mPreloadCells.end();)
        {
            if (it->second.operation)
                it->second.operation->abort = true;
            mPreloadCells.erase(it++);
        }
    }

    void CellPreloader::setExpiryDelay(double expiryDelay)
    {
        mExpiryDelay = expiryDelay;
    }

    void CellPreloader::setMinCacheSize(unsigned int num)
    {
        mMinCacheSize = num;
    }

    void CellPreloader::setMaxCacheSize(unsigned int num)
    {
        mMaxCacheSize = num;
    }

    unsigned int CellPreloader::getMaxCacheSize() const
    {
        return mMaxCacheSize;
    }

    vsg::ref_ptr<vsg::OperationThreads> CellPreloader::getCompileThreads()
    {
        return mCompileThreads;
    }
}
