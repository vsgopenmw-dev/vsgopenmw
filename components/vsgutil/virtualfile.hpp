#ifndef VSGOPENMW_VSGUTIL_VIRTUALFILE_H
#define VSGOPENMW_VSGUTIL_VIRTUALFILE_H

#include <vsg/io/ReaderWriter.h>

namespace vsgUtil
{
    class VirtualFile : public vsg::ReaderWriter
    {
    public:
        vsg::ref_ptr<vsg::Object> object;
        vsg::Path filename;
        vsg::ref_ptr<vsg::Object> read(const vsg::Path &path, vsg::ref_ptr<const vsg::Options>) const override
        {
            if (vsg::simpleFilename(path) == filename)
                return object;
            return {};
        }
    };
}

#endif
