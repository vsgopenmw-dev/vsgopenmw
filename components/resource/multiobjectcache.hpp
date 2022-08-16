#ifndef OPENMW_COMPONENTS_MULTIOBJECTCACHE_H
#define OPENMW_COMPONENTS_MULTIOBJECTCACHE_H

#include <map>
#include <string>
#include <mutex>

#include <vsg/core/Object.h>

namespace Resource
{

    /// @brief Cache for "non reusable" objects.
    class MultiObjectCache : public vsg::Object
    {
    public:
        MultiObjectCache();
        ~MultiObjectCache();

        void removeUnreferencedObjectsInCache();

        /** Remove all objects from the cache. */
        void clear();

        void addEntryToObjectCache(const std::string& filename, vsg::Object* object);

        /** Take an Object from cache. Return nullptr if no object found. */
        vsg::ref_ptr<vsg::Object> takeFromObjectCache(const std::string& fileName);

        unsigned int getCacheSize() const;

    protected:

        typedef std::multimap<std::string, vsg::ref_ptr<vsg::Object> >             ObjectCacheMap;

        ObjectCacheMap                          _objectCache;
        mutable std::mutex                      _objectCacheMutex;

    };

}

#endif
