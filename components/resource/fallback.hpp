#ifndef VSGOPENMW_RESOURCE_FALLBACK_H
#define VSGOPENMW_RESOURCE_FALLBACK_H

#include <vsg/io/ReaderWriter.h>

namespace Resource
{
    /*
     * Replaces missing or erroneous files.
     */
    class Fallback : public vsg::ReaderWriter
    {
        vsg::ref_ptr<vsg::Object> readFallback(vsg::ref_ptr<const vsg::Options> options) const;
    public:
        std::vector<std::string> fallbackFiles;
        vsg::ref_ptr<vsg::Object> fallbackObject;
        vsg::ref_ptr<vsg::Object> read(
            const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options = {}) const override;
    };
}

#endif
