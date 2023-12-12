#ifndef VSGOPENMW_VSGUTIL_NAME_H
#define VSGOPENMW_VSGUTIL_NAME_H

#include <vsg/core/Object.h>

namespace vsgUtil
{
    static const std::string sNameKey = "name";

    /*
     * Optionally attaches names to objects.
     */
    inline void setName(vsg::Object& o, /*const& */ std::string name)
    {
        o.setValue(sNameKey, name);
    }

    const std::string& getName(const vsg::Object& n);
}

#endif
