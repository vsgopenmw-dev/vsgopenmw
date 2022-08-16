#ifndef VSGOPENMW_ANIMATION_TAGS_H
#define VSGOPENMW_ANIMATION_TAGS_H

#include <algorithm>
#include <map>
#include <set>
#include <string>

#include <components/vsgutil/attachable.hpp>

namespace Anim
{
    /*
     * Annotates time track.
     */
    class Tags : public vsgUtil::Attachable<Tags>
    {
    public:
        static const std::string sAttachKey;
        auto begin() const noexcept
        {
            return mTags.begin();
        }
        auto end() const noexcept
        {
            return mTags.end();
        }
        auto rbegin() const noexcept
        {
            return mTags.rbegin();
        }
        auto rend() const noexcept
        {
            return mTags.rend();
        }
        auto lowerBound(float time) const
        {
            return mTags.lower_bound(time);
        }
        auto upperBound(float time) const
        {
            return mTags.upper_bound(time);
        }
        void emplace(float time, std::string&& textKey)
        {
            const auto separator = textKey.find(": ");
            if (separator != std::string::npos)
                mGroups.emplace(textKey.substr(0, separator));
            mTags.emplace(time, std::move(textKey));
        }
        bool empty() const noexcept
        {
            return mTags.empty();
        }
        auto findGroup(std::string_view group) const
        {
            auto matchGroup = [&group](const std::multimap<float, std::string>::value_type& value)     -> bool
            {
                return value.second.compare(0, group.size(), group) == 0 &&
                        value.second.compare(group.size(), 2, ": ") == 0;
            };
            return std::find_if(mTags.begin(), mTags.end(), matchGroup);
        }
        bool hasGroup(std::string_view group) const
        {
            return mGroups.count(group) > 0;
        }
        using ConstIterator = std::multimap<float, std::string>::const_iterator;
    private:
        std::set<std::string, std::less<>> mGroups;
        std::multimap<float, std::string> mTags;
    };
}

#endif
