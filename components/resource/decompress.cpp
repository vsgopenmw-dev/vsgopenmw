#include "decompress.hpp"

#include <components/vsgutil/decompress.hpp>

namespace Resource
{
    vsg::ref_ptr<vsg::Object> Decompress::read(
        const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
    {
        return decompress(CompositeReaderWriter::read(filename, options));
    }

    vsg::ref_ptr<vsg::Object> Decompress::read(
        std::istream& stream, vsg::ref_ptr<const vsg::Options> options) const
    {
        return decompress(CompositeReaderWriter::read(stream, options));
    }

    vsg::ref_ptr<vsg::Object> Decompress::decompress(vsg::ref_ptr<vsg::Object> obj) const
    {
        if (!obj)
            return {};
        if (auto data = obj->cast<vsg::Data>())
        {
            if (vsgUtil::isCompressed(*data))
                return vsgUtil::decompressImage(*data);
        }
        return obj;
    }
}
