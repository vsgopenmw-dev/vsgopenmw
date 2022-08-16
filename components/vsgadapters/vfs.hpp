#ifndef VSGOPENMW_VSGADAPTERS_VFS_H
#define VSGOPENMW_VSGADAPTERS_VFS_H

#include <vsg/io/ReaderWriter.h>

namespace VFS
{
    class Manager;
}
namespace vsgAdapters
{
    /*
     * Adapts read(Path) to read(istream) through VFS.
     */
    class vfs : public vsg::CompositeReaderWriter
    {
    public:
        vfs(const VFS::Manager &vfs, const vsg::ReaderWriters &children = {}) : mVfs(vfs)
        {
            readerWriters = children;
        }
        vsg::ref_ptr<vsg::Object> read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options = {}) const override;

   private:
        const VFS::Manager &mVfs;
   };
}

#endif
