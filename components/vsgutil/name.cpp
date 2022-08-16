#include "name.hpp"

#include <vsg/core/Value.h>

namespace vsgUtil
{
    static const std::string sNoName = "";
    const std::string &getName(const vsg::Node &n)
    {
        auto val = n.getObject<vsg::stringValue>(sNameKey);
        return val ? val->value() : sNoName;
    }
}
