#include "readnode.hpp"

#include <stdexcept>

#include <vsg/io/read.h>
#include <vsg/nodes/Node.h>

namespace vsgUtil
{
    vsg::ref_ptr<vsg::Node> readNode(
        const std::string& path, vsg::ref_ptr<const vsg::Options> options)
    {
        if (auto node = vsg::read_cast<vsg::Node>(path, options))
            return node;
        throw std::runtime_error("!readNode(\"" + path + "\")");
    }
}
