#include "navmeshtilescache.hpp"

#include <cstring>

namespace DetourNavigator
{
    NavMeshTilesCache::NavMeshTilesCache(const std::size_t maxNavMeshDataSize)
        : mMaxNavMeshDataSize(maxNavMeshDataSize), mUsedNavMeshDataSize(0), mFreeNavMeshDataSize(0),
          mHitCount(0), mGetCount(0) {}

    NavMeshTilesCache::Value NavMeshTilesCache::get(const AgentBounds& agentBounds, const TilePosition& changedTile,
        const RecastMesh& recastMesh)
    {
        const std::lock_guard<std::mutex> lock(mMutex);

        ++mGetCount;

        const auto tile = mValues.find(std::tie(agentBounds, changedTile, recastMesh));
        if (tile == mValues.end())
            return Value();

        acquireItemUnsafe(tile->second);

        ++mHitCount;

        return Value(*this, tile->second);
    }

    NavMeshTilesCache::Value NavMeshTilesCache::set(const AgentBounds& agentBounds, const TilePosition& changedTile,
        const RecastMesh& recastMesh, std::unique_ptr<PreparedNavMeshData>&& value)
    {
        const auto itemSize = sizeof(RecastMesh) + getSize(recastMesh)
            + (value == nullptr ? 0 : sizeof(PreparedNavMeshData) + getSize(*value));

        const std::lock_guard<std::mutex> lock(mMutex);

        if (itemSize > mFreeNavMeshDataSize + (mMaxNavMeshDataSize - mUsedNavMeshDataSize))
            return Value();

        while (!mFreeItems.empty() && mUsedNavMeshDataSize + itemSize > mMaxNavMeshDataSize)
            removeLeastRecentlyUsed();

        RecastMeshData key {recastMesh.getMesh(), recastMesh.getWater(),
                    recastMesh.getHeightfields(), recastMesh.getFlatHeightfields()};

        const auto iterator = mFreeItems.emplace(mFreeItems.end(), agentBounds, changedTile, std::move(key), itemSize);
        const auto emplaced = mValues.emplace(std::make_tuple(agentBounds, changedTile, std::cref(iterator->mRecastMeshData)), iterator);

        if (!emplaced.second)
        {
            mFreeItems.erase(iterator);
            acquireItemUnsafe(emplaced.first->second);
            ++mGetCount;
            ++mHitCount;
            return Value(*this, emplaced.first->second);
        }

        iterator->mPreparedNavMeshData = std::move(value);
        ++iterator->mUseCount;
        mUsedNavMeshDataSize += itemSize;
        mBusyItems.splice(mBusyItems.end(), mFreeItems, iterator);

        return Value(*this, iterator);
    }

    NavMeshTilesCache::Stats NavMeshTilesCache::getStats() const
    {
        Stats result;
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            result.mNavMeshCacheSize = mUsedNavMeshDataSize;
            result.mUsedNavMeshTiles = mBusyItems.size();
            result.mCachedNavMeshTiles = mFreeItems.size();
            result.mHitCount = mHitCount;
            result.mGetCount = mGetCount;
        }
        return result;
    }

    void reportStats(const NavMeshTilesCache::Stats& stats, unsigned int frameNumber, osg::Stats& out)
    {
        /*
        out.setAttribute(frameNumber, "NavMesh CacheSize", static_cast<double>(stats.mNavMeshCacheSize));
        out.setAttribute(frameNumber, "NavMesh UsedTiles", static_cast<double>(stats.mUsedNavMeshTiles));
        out.setAttribute(frameNumber, "NavMesh CachedTiles", static_cast<double>(stats.mCachedNavMeshTiles));
        if (stats.mGetCount > 0)
            out.setAttribute(frameNumber, "NavMesh CacheHitRate", static_cast<double>(stats.mHitCount) / stats.mGetCount * 100.0);
            */
    }

    void NavMeshTilesCache::removeLeastRecentlyUsed()
    {
        const auto& item = mFreeItems.back();

        const auto value = mValues.find(std::tie(item.mAgentBounds, item.mChangedTile, item.mRecastMeshData));
        if (value == mValues.end())
            return;

        mUsedNavMeshDataSize -= item.mSize;
        mFreeNavMeshDataSize -= item.mSize;

        mValues.erase(value);
        mFreeItems.pop_back();
    }

    void NavMeshTilesCache::acquireItemUnsafe(ItemIterator iterator)
    {
        if (++iterator->mUseCount > 1)
            return;

        mBusyItems.splice(mBusyItems.end(), mFreeItems, iterator);
        mFreeNavMeshDataSize -= iterator->mSize;
    }

    void NavMeshTilesCache::releaseItem(ItemIterator iterator)
    {
        if (--iterator->mUseCount > 0)
            return;

        const std::lock_guard<std::mutex> lock(mMutex);

        mFreeItems.splice(mFreeItems.begin(), mBusyItems, iterator);
        mFreeNavMeshDataSize += iterator->mSize;
    }
}
