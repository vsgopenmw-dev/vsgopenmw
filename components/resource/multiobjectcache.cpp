#include "multiobjectcache.hpp"

#include <iostream>
#include <vector>

namespace Resource
{

    MultiObjectCache::MultiObjectCache()
    {

    }

    MultiObjectCache::~MultiObjectCache()
    {

    }

    void MultiObjectCache::removeUnreferencedObjectsInCache()
    {
        std::vector<vsg::ref_ptr<vsg::Object> > objectsToRemove;
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);

            // Remove unreferenced entries from object cache
            ObjectCacheMap::iterator oitr = _objectCache.begin();
            while(oitr != _objectCache.end())
            {
                if (oitr->second->referenceCount() <= 1)
                {
                    objectsToRemove.push_back(oitr->second);
                    _objectCache.erase(oitr++);
                }
                else
                {
                    ++oitr;
                }
            }
        }

        // note, actual unref happens outside of the lock
        objectsToRemove.clear();
    }

    void MultiObjectCache::clear()
    {
        std::lock_guard<std::mutex> lock(_objectCacheMutex);
        _objectCache.clear();
    }

    void MultiObjectCache::addEntryToObjectCache(const std::string &filename, vsg::Object *object)
    {
        if (!object)
        {
            std::cerr << " trying to add NULL object to cache for " << filename << std::endl;
            return;
        }
        std::lock_guard<std::mutex> lock(_objectCacheMutex);
        _objectCache.insert(std::make_pair(filename, object));
    }

    vsg::ref_ptr<vsg::Object> MultiObjectCache::takeFromObjectCache(const std::string &fileName)
    {
        std::lock_guard<std::mutex> lock(_objectCacheMutex);
        ObjectCacheMap::iterator found = _objectCache.find(fileName);
        if (found == _objectCache.end())
            return vsg::ref_ptr<vsg::Object>();
        else
        {
            vsg::ref_ptr<vsg::Object> object = found->second;
            _objectCache.erase(found);
            return object;
        }
    }

    unsigned int MultiObjectCache::getCacheSize() const
    {
        std::lock_guard<std::mutex> lock(_objectCacheMutex);
        return _objectCache.size();
    }

}
