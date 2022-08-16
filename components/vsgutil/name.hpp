#ifndef VSGOPENMW_VSGUTIL_NAME_H
#define VSGOPENMW_VSGUTIL_NAME_H

#include <vsg/nodes/Node.h>

namespace vsgUtil
{
    static const std::string sNameKey = "name";

    /*
     * Optionally attaches names to nodes.
     */
    inline void setName(vsg::Node &n, /*const& */std::string name)
    {
        n.setValue(sNameKey, name);
    }

    const std::string &getName(const vsg::Node &n);
}

#endif
