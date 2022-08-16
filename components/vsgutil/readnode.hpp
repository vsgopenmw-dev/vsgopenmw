#ifndef VSGOPENMW_VSGUTIL_READNODE_H
#define VSGOPENMW_VSGUTIL_READNODE_H

#include <vsg/io/Options.h>

namespace vsgUtil
{
    /*
     * Reads a model. 
     * [[or the default model on failure.]]
     */
    vsg::ref_ptr<vsg::Node> readNode(const std::string &path, vsg::ref_ptr<const vsg::Options> options, bool throwOnFailure = true);

    //readActor
}

#endif
