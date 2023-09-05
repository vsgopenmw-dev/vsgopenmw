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
        Object getOrCreate(const Key& key, const Creator& creator) const
        {
            {
                std::lock_guard<std::mutex> guard(mMutex);
                if (auto itr = mMap.find(key); itr != mMap.end())
                    return itr->second;
            }
            auto val = creator.create(key);
            std::lock_guard<std::mutex> guard(mMutex);
            mMap[key] = val;
            return val;
        }
        template <class Condition>
        void removeObjects(Condition condition) const
        {
            std::lock_guard<std::mutex> guard(this->mMutex);
            for (auto it = this->mMap.begin(); it != this->mMap.end(); )
            {
                if (condition(it->second))
                    it = this->mMap.erase(it);
                else
                    ++it;
            }
        }
        size_t size() const
        {
            std::lock_guard<std::mutex> guard(this->mMutex);
            return this->mMap.size();
        }
    };

    template <class Key, class Object>
    class RefCache : public Cache<Key, vsg::ref_ptr<Object>>
    {
    public:
        /*
         * Removes no longer required objects.
         */
        void prune()
        {
            this->removeObjects([](const vsg::ref_ptr<Object>& o) -> bool { return o->referenceCount() <= 1; });
        }
    };
}

#endif
