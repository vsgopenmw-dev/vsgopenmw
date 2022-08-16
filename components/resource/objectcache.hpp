#ifndef OPENMW_COMPONENTS_RESOURCE_OBJECTCACHE
#define OPENMW_COMPONENTS_RESOURCE_OBJECTCACHE

#include <vsg/core/Object.h>
#include <vsg/core/ref_ptr.h>

#include <string>
#include <map>
#include <mutex>

namespace Resource {

template <typename KeyType>
class GenericObjectCache : public vsg::Object
{
    public:

        /** For each object in the cache which has an reference count greater than 1
          * (and therefore referenced by elsewhere in the application) set the time stamp
          * for that object in the cache to specified time.
          * This would typically be called once per frame by applications which are doing database paging,
          * and need to prune objects that are no longer required.
          * The time used should be taken from the FrameStamp::getReferenceTime().*/
        void updateTimeStampOfObjectsInCacheWithExternalReferences(double referenceTime)
        {
            // look for objects with external references and update their time stamp.
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            for(typename ObjectCacheMap::iterator itr=_objectCache.begin(); itr!=_objectCache.end(); ++itr)
            {
                // If ref count is greater than 1, the object has an external reference.
                // If the timestamp is yet to be initialized, it needs to be updated too.
                if (itr->second.first->referenceCount()>1 || itr->second.second == 0.0)
                    itr->second.second = referenceTime;
            }
        }

        /** Removed object in the cache which have a time stamp at or before the specified expiry time.
          * This would typically be called once per frame by applications which are doing database paging,
          * and need to prune objects that are no longer required, and called after the a called
          * after the call to updateTimeStampOfObjectsInCacheWithExternalReferences(expirtyTime).*/
        void removeExpiredObjectsInCache(double expiryTime)
        {
            std::vector<vsg::ref_ptr<vsg::Object> > objectsToRemove;
            {
                std::lock_guard<std::mutex> lock(_objectCacheMutex);
                // Remove expired entries from object cache
                typename ObjectCacheMap::iterator oitr = _objectCache.begin();
                while(oitr != _objectCache.end())
                {
                    if (oitr->second.second<=expiryTime)
                    {
                        objectsToRemove.push_back(oitr->second.first);
                        _objectCache.erase(oitr++);
                    }
                    else
                        ++oitr;
                }
            }
            // note, actual unref happens outside of the lock
            objectsToRemove.clear();
        }

        /** Remove all objects in the cache regardless of having external references or expiry times.*/
        void clear()
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            _objectCache.clear();
        }

        /** Add a key,object,timestamp triple to the Registry::ObjectCache.*/
        void addEntryToObjectCache(const KeyType& key, vsg::Object* object, double timestamp = 0.0)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            _objectCache[key]=ObjectTimeStampPair(object,timestamp);
        }

        /** Remove Object from cache.*/
        void removeFromObjectCache(const KeyType& key)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            typename ObjectCacheMap::iterator itr = _objectCache.find(key);
            if (itr!=_objectCache.end()) _objectCache.erase(itr);
        }

        /** Get an ref_ptr<Object> from the object cache*/
        vsg::ref_ptr<vsg::Object> getRefFromObjectCache(const KeyType& key)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            typename ObjectCacheMap::iterator itr = _objectCache.find(key);
            if (itr!=_objectCache.end())
                return itr->second.first;
            else return {};
        }

        /** Check if an object is in the cache, and if it is, update its usage time stamp. */
        bool checkInObjectCache(const KeyType& key, double timeStamp)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            typename ObjectCacheMap::iterator itr = _objectCache.find(key);
            if (itr!=_objectCache.end())
            {
                itr->second.second = timeStamp;
                return true;
            }
            else return false;
        }

        /** call node->accept(nv); for all nodes in the objectCache.
        void accept(osg::NodeVisitor& nv)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            for(typename ObjectCacheMap::iterator itr = _objectCache.begin(); itr != _objectCache.end(); ++itr)
            {
                vsg::Object* object = itr->second.first.get();
                if (object)
                {
                    osg::Node* node = dynamic_cast<osg::Node*>(object);
                    if (node)
                        node->accept(nv);
                }
            }
        }
*/
        /** call operator()(KeyType, vsg::Object*) for each object in the cache. */
        template <class Functor>
        void call(Functor& f)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            for (typename ObjectCacheMap::iterator it = _objectCache.begin(); it != _objectCache.end(); ++it)
                f(it->first, it->second.first.get());
        }

        /** Get the number of objects in the cache. */
        unsigned int getCacheSize() const
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            return _objectCache.size();
        }

    protected:

        virtual ~GenericObjectCache() {}

        using ObjectTimeStampPair = std::pair<vsg::ref_ptr<vsg::Object>, double >;
        using ObjectCacheMap = std::map<KeyType, ObjectTimeStampPair >;

        ObjectCacheMap                          _objectCache;
        mutable std::mutex                      _objectCacheMutex;

};

class ObjectCache : public GenericObjectCache<std::string>
{
};

}

#endif
