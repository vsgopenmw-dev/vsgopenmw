#ifndef OPENMW_COMPONENTS_RESOURCE_OBJECTCACHE
#define OPENMW_COMPONENTS_RESOURCE_OBJECTCACHE

#include <vsg/core/Object.h>
#include <vsg/core/ref_ptr.h>

#include <map>
#include <mutex>
#include <optional>
#include <string>

namespace Resource
{

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
            for (typename ObjectCacheMap::iterator itr = _objectCache.begin(); itr != _objectCache.end(); ++itr)
            {
                // If ref count is greater than 1, the object has an external reference.
                // If the timestamp is yet to be initialized, it needs to be updated too.
                if ((itr->second.mValue != nullptr && itr->second.mValue->referenceCount() > 1)
                    || itr->second.mLastUsage == 0.0)
                    itr->second.mLastUsage = referenceTime;
            }
        }

        /** Removed object in the cache which have a time stamp at or before the specified expiry time.
         * This would typically be called once per frame by applications which are doing database paging,
         * and need to prune objects that are no longer required, and called after the a called
         * after the call to updateTimeStampOfObjectsInCacheWithExternalReferences(expirtyTime).*/
        void removeExpiredObjectsInCache(double expiryTime)
        {
            std::vector<vsg::ref_ptr<vsg::Object>> objectsToRemove;
            {
                std::lock_guard<std::mutex> lock(_objectCacheMutex);
                // Remove expired entries from object cache
                typename ObjectCacheMap::iterator oitr = _objectCache.begin();
                while (oitr != _objectCache.end())
                {
                    if (oitr->second.mLastUsage <= expiryTime)
                    {
                        if (oitr->second.mValue != nullptr)
                            objectsToRemove.push_back(std::move(oitr->second.mValue));
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
            _objectCache[key] = Item{ object, timestamp };
        }

        /** Remove Object from cache.*/
        void removeFromObjectCache(const KeyType& key)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            typename ObjectCacheMap::iterator itr = _objectCache.find(key);
            if (itr != _objectCache.end())
                _objectCache.erase(itr);
        }

        /** Get an ref_ptr<Object> from the object cache*/
        vsg::ref_ptr<vsg::Object> getRefFromObjectCache(const KeyType& key)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            typename ObjectCacheMap::iterator itr = _objectCache.find(key);
            if (itr != _objectCache.end())
                return itr->second.mValue;
            else
                return {};
        }

        std::optional<vsg::ref_ptr<vsg::Object>> getRefFromObjectCacheOrNone(const KeyType& key)
        {
            const std::lock_guard<std::mutex> lock(_objectCacheMutex);
            const auto it = _objectCache.find(key);
            if (it == _objectCache.end())
                return std::nullopt;
            return it->second.mValue;
        }

        /** Check if an object is in the cache, and if it is, update its usage time stamp. */
        bool checkInObjectCache(const KeyType& key, double timeStamp)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            typename ObjectCacheMap::iterator itr = _objectCache.find(key);
            if (itr != _objectCache.end())
            {
                itr->second.mLastUsage = timeStamp;
                return true;
            }
            else
                return false;
        }

        /** call node->accept(nv); for all nodes in the objectCache.
        void accept(osg::NodeVisitor& nv)
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            for(typename ObjectCacheMap::iterator itr = _objectCache.begin(); itr != _objectCache.end(); ++itr)
            {
                if (osg::Object* object = itr->second.mValue.get())
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
                f(it->first, it->second.mValue.get());
        }

        /** Get the number of objects in the cache. */
        unsigned int getCacheSize() const
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);
            return _objectCache.size();
        }

        template <class K>
        std::optional<std::pair<KeyType, vsg::ref_ptr<vsg::Object>>> lowerBound(K&& key)
        {
            const std::lock_guard<std::mutex> lock(_objectCacheMutex);
            const auto it = _objectCache.lower_bound(std::forward<K>(key));
            if (it == _objectCache.end())
                return std::nullopt;
            return std::pair(it->first, it->second.mValue);
        }

    protected:
        struct Item
        {
            vsg::ref_ptr<vsg::Object> mValue;
            double mLastUsage;
        };

        virtual ~GenericObjectCache() {}

<<<<<<< HEAD
        using ObjectCacheMap = std::map<KeyType, Item, std::less<>>;
=======
        using ObjectTimeStampPair = std::pair<vsg::ref_ptr<vsg::Object>, double>;
        using ObjectCacheMap = std::map<KeyType, ObjectTimeStampPair>;
>>>>>>> 954897300b (vsgopenmw-openmw)

        ObjectCacheMap _objectCache;
        mutable std::mutex _objectCacheMutex;
    };

    class ObjectCache : public GenericObjectCache<std::string>
    {
    };

}

#endif
