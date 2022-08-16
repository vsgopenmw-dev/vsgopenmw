#ifndef VSGOPENNW_ANIMATION_META_H
#define VSGOPENNW_ANIMATION_META_H

#include "contents.hpp"

#include <components/vsgutil/attachable.hpp>

namespace Anim
{
    /*
     * Describes subgraph.
     */
    class Meta : public vsgUtil::Attachable<Meta>
    {
    public:
        Meta(Contents c) : contents(c) {}
        static const std::string sAttachKey;
        Contents contents;
        static Anim::Contents getContents(vsg::Object &obj)
        {
            if (auto meta = get(obj))
                return meta->contents;
            return {};
        }
    };
}

#endif
