#include "readimage.hpp"

#include <stdexcept>

#include <vsg/io/read.h>

namespace vsgUtil
{
    vsg::ref_ptr<vsg::Data> readImage(const std::string& path, vsg::ref_ptr<const vsg::Options> options)
    {
        if (auto data = readOptionalImage(path, options))
            return data;
        throw std::runtime_error("!readImage(\"" + path + "\")");
    }

    vsg::ref_ptr<vsg::Data> readOptionalImage(const std::string& path, vsg::ref_ptr<const vsg::Options> options)
    {
        return vsg::read_cast<vsg::Data>(path, options);
    }
}
