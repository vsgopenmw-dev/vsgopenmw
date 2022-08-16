#ifndef VSGOPENMW_VSGUTIL_READSHADER_H
#define VSGOPENMW_VSGUTIL_READSHADER_H

#include <vsg/io/Options.h>
#include <vsg/state/ShaderStage.h>

namespace vsgUtil
{
    /*
     * Reads vsg::ShaderStage or throws.
     */
    vsg::ref_ptr<vsg::ShaderStage> readShader(const std::string &path, vsg::ref_ptr<const vsg::Options> options);
}

#endif
