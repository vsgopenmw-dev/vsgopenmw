#ifndef OPENMW_COMPONENTS_RESOURCE_MANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_MANAGER_H

#include <vsg/core/ref_ptr.h>

#include "objectcache.hpp"

namespace VFS
{
    class Manager;
}

namespace osg
{
    class Stats;
}

namespace Resource
{

    class BaseResourceManager
    {
    public:
        virtual ~BaseResourceManager() {}
        virtual void updateCache(double referenceTime) {}
        virtual void clearCache() {}
        virtual void setExpiryDelay(double expiryDelay) {}
        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) const {}
    };

    /// @brief Base class for managers that require a virtual file system and object cache.
    /// @par This base class implements clearing of the cache, but populating it and what it's used for is up to the
    /// individual sub classes.
    template <class KeyType>
    class GenericResourceManager : public BaseResourceManager
    {
    public:
        typedef GenericObjectCache<KeyType> CacheType;

        explicit GenericResourceManager(const VFS::Manager* vfs, double expiryDelay)
            : mVFS(vfs)
            , mCache(new CacheType)
            , mExpiryDelay(expiryDelay)
        {
        }

        virtual ~GenericResourceManager() = default;

        /// Clear cache entries that have not been referenced for longer than expiryDelay.
        void updateCache(double referenceTime) override
        {
            mCache->updateTimeStampOfObjectsInCacheWithExternalReferences(referenceTime);
            mCache->removeExpiredObjectsInCache(referenceTime - mExpiryDelay);
        }

        /// Clear all cache entries.
        void clearCache() override { mCache->clear(); }

        /// How long to keep objects in cache after no longer being referenced.
        void setExpiryDelay(double expiryDelay) final { mExpiryDelay = expiryDelay; }
        double getExpiryDelay() const { return mExpiryDelay; }

        const VFS::Manager* getVFS() const { return mVFS; }

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override {}

    protected:
        const VFS::Manager* mVFS;
        vsg::ref_ptr<CacheType> mCache;
        double mExpiryDelay;
    };

    class ResourceManager : public GenericResourceManager<std::string>
    {
    public:
        explicit ResourceManager(const VFS::Manager* vfs, double expiryDelay)
            : GenericResourceManager<std::string>(vfs, expiryDelay)
        {
        }
    };

}

#endif
