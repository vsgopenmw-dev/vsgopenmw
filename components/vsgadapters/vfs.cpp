#include "vfs.hpp"

#include <istream>

#include <components/vfs/manager.hpp>

namespace
{
    vsg::ref_ptr<vsg::Object> readImpl(const vsg::Path &filename, vsg::ref_ptr<const vsg::Options> options, const vsg::ReaderWriters &readers, const VFS::Manager &vfs)
    {
        // ;
        if (!vfs.exists(filename.c_str()))
            return {};

        auto stream = vfs./*search*/get(filename.c_str());
        auto ext = vsg::lowerCaseFileExtension(filename);

        auto extOptions = vsg::Options::create(*options);
        extOptions->extensionHint = ext;
        extOptions->setValue("filename", std::string(filename));

        for (auto &reader : readers)
        {
            /*
            Features features;
            if (!reader->getFeatures(features))
                continue;
            auto feature = features.extensionFeatureMap.find(ext);
            if (feature == features.extensionFeatureMap.end() || !(feature->second & READ_ISTREAM))
                continue;
            */
            return reader->read(*stream, extOptions);
        }
        //std::cerr << "extensionFeatureMap.find(" + ext + ") == extensionFeatureMap.end()" << std::endl;
        return {};
    }
}

namespace vsgAdapters
{
    vsg::ref_ptr<vsg::Object> vfs::read(const vsg::Path &filename, vsg::ref_ptr<const vsg::Options> options) const
    {
        if (options->findFileCallback)
            return readImpl(options->findFileCallback(filename, options), options, this->readerWriters, mVfs);
        return readImpl(filename, options, this->readerWriters, mVfs);
   }
}


