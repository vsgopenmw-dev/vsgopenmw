#ifndef VSGOPENMW_VSGUTIL_VIRTUALFILE_H
#define VSGOPENMW_VSGUTIL_VIRTUALFILE_H

#include <vsg/io/ReaderWriter.h>

namespace vsgUtil
{
    class VirtualFiles : public vsg::ReaderWriter
    {
    public:
        std::map<vsg::Path, vsg::ref_ptr<vsg::Object>> objects;
        vsg::ref_ptr<vsg::Object> read(const vsg::Path& path, vsg::ref_ptr<const vsg::Options>) const override
        {
            auto i = objects.find(path);
            if (i != objects.end())
                return i->second;
            return {};
        }
    };
}

#endif
