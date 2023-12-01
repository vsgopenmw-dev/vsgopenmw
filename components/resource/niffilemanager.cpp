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

    NifFileManager::NifFileManager(const VFS::Manager* vfs)
        // NIF files aren't needed any more once the converted objects are cached in SceneManager / BulletShapeManager,
        // so no point in using an expiry delay.
        : ResourceManager(vfs, 0)
    {
    }

    NifFileManager::~NifFileManager() {}

    Nif::NIFFilePtr NifFileManager::get(const std::string& name)
    {
        vsg::ref_ptr<vsg::Object> obj = mCache->getRefFromObjectCache(name);
        if (obj)
            return static_cast<NifFileHolder*>(obj.get())->mNifFile;
        else
        {
            auto file = std::make_shared<Nif::NIFFile>(name);
            Nif::Reader reader(*file);
            reader.parse(mVFS->get(name));
            obj = new NifFileHolder(file);
            mCache->addEntryToObjectCache(name, obj);
            return file;
        }
    }

    void NifFileManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const {}
}
