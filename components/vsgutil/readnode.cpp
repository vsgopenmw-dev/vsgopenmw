#include "readnode.hpp"

#include <vsg/io/read.h>
#include <vsg/nodes/Node.h>
#include <stdexcept>

namespace vsgUtil
{
    vsg::ref_ptr<vsg::Node> readNode(const std::string &path, vsg::ref_ptr<const vsg::Options> options, bool throwOnFailure)
    {
        auto node = vsg::read_cast<vsg::Node>(path, options);
        if (!node && throwOnFailure)
            throw std::runtime_error("!vsg::read_cast<vsg::Node>(\""+path+"\")");
        return node;
    }
}
