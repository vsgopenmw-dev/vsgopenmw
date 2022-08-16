#include "readshader.hpp"

#include <vsg/io/read.h>

namespace vsgUtil
{
    vsg::ref_ptr<vsg::ShaderStage> readShader(const std::string &path, vsg::ref_ptr<const vsg::Options> options)
    {
        if (auto shaderStage = vsg::read_cast<vsg::ShaderStage>(path, options))
            return shaderStage;
        throw std::runtime_error("!vsg::read_cast<vsg::ShaderStage>(\"" + path + "\")");
    }
}
