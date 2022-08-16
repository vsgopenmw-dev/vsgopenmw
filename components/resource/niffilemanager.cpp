#include "niffilemanager.hpp"

#include <istream>

#include <components/vfs/manager.hpp>

#include "objectcache.hpp"

namespace Resource
{
    class NifFileHolder : public vsg::Object
    {
    public:
        NifFileHolder(const Nif::NIFFilePtr& file)
            : mNifFile(file)
        {
        }
        Nif::NIFFilePtr mNifFile;
    };

    NifFileManager::NifFileManager(const VFS::Manager *vfs)
        : ResourceManager(vfs)
    {
    }

    NifFileManager::~NifFileManager()
    {
    }

    Nif::NIFFilePtr NifFileManager::get(const std::string &name)
    {
        vsg::ref_ptr<vsg::Object> obj = mCache->getRefFromObjectCache(name);
        if (obj)
            return static_cast<NifFileHolder*>(obj.get())->mNifFile;
        else
        {
            Nif::NIFFilePtr file (new Nif::NIFFile(mVFS->get(name), name));
            obj = new NifFileHolder(file);
            mCache->addEntryToObjectCache(name, obj);
            return file;
        }
    }

    void NifFileManager::reportStats(unsigned int frameNumber, osg::Stats *stats) const
    {
    }
}
