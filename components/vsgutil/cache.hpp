#ifndef VSGOPENMW_VSGUTIL_CACHE_H
#define VSGOPENMW_VSGUTIL_CACHE_H

#include <vsg/core/ref_ptr.h>

#include <map>
#include <mutex>

namespace vsgUtil
{
    /*
     * Stores created objects for reuse.
     */
    template <class Key, class Object>
    class Cache
    {
    protected:
        using Map = std::map<Key, Object>;
        mutable Map mMap;
        mutable std::mutex mMutex;

    public:
        template <class Creator>
        Object getOrCreate(const Key &key, const Creator &creator) const
        {
            {
                std::lock_guard<std::mutex> guard(mMutex);
                if (auto itr = mMap.find(key); itr != mMap.end()) return itr->second;
            }
            auto val = creator.create(key);
            std::lock_guard<std::mutex> guard(mMutex);
            mMap[key] = val;
            return val;
        }
    };

    template <class Key, class Object>
    using RefCache = Cache<Key, vsg::ref_ptr<Object>>;
}

#endif
