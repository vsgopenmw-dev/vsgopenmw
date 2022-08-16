#ifndef VSGOPENMW_VSGUTIL_ID_H
#define VSGOPENMW_VSGUTIL_ID_H

#include "attachable.hpp"

namespace vsgUtil
{
    /*
     * Optionally identifies object.
     */
    class ID : public vsgUtil::Attachable<ID>
    {
    public:
        ID(int i) : id(i) {}
        static const std::string sAttachKey;
        int id{};
    };

    inline void setID(vsg::Object &o, int i)
    {
        vsg::ref_ptr{new ID(i)}->attachTo(o);
    }
}

#endif
