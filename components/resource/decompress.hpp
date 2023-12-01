#ifndef VSGOPENMW_RESOURCE_DECOMPRESS_H
#define VSGOPENMW_RESOURCE_DECOMPRESS_H

#include <vsg/io/ReaderWriter.h>

namespace Resource
{
    /*
     * Passes read result of child ReaderWriters through vsgUtil::decompress.
     */
    class Decompress : public vsg::CompositeReaderWriter
    {
        vsg::ref_ptr<vsg::Object> decompress(vsg::ref_ptr<vsg::Object> obj) const;
    public:
        vsg::ref_ptr<vsg::Object> read(
            const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options = {}) const override;
        vsg::ref_ptr<vsg::Object> read(
            std::istream&, vsg::ref_ptr<const vsg::Options> options = {}) const override;
    };
}

#endif
