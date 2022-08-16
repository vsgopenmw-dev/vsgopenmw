#include "resourcesystem.hpp"

#include <vsgXchange/images.h>
#include <vsgXchange/glsl.h>
#include <vsg/io/Options.h>
#include <vsg/utils/SharedObjects.h>

#include <components/vsgadapters/vfs.hpp>
#include <components/vsgadapters/nif/nif.hpp>
#include <components/vsgadapters/nif/kf.hpp>
#include <components/pipeline/builder.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "niffilemanager.hpp"
#include "shadersettings.hpp"

namespace
{
    vsg::ref_ptr<const vsg::Options> createShaderOptions(const std::string &shaderPath)
    {
        auto options = vsg::Options::create();
        auto shaderSettings = vsg::ref_ptr{new Resource::ShaderSettings};
        options->readerWriters = {shaderSettings, vsgXchange::glsl::create()};
        options->paths = {shaderPath};
        return options;
    }

    vsg::ref_ptr<const vsg::Options> createImageOptions(const VFS::Manager *vfs)
    {
        auto images = vsg::ref_ptr{new vsgXchange::images};
        auto options = vsg::Options::create();
        options->readerWriters = {vsg::ref_ptr{new vsgAdapters::vfs(*vfs, {images})}};
        options->sharedObjects = vsg::SharedObjects::create();
        return options;
    }

    vsg::ref_ptr<const vsg::Options> createTextureOptions(const VFS::Manager *vfs, const vsg::Options &imageOptions)
    {
        auto textureOptions = vsg::Options::create(imageOptions);
        textureOptions->paths = {"textures"};
        textureOptions->sharedObjects = vsg::SharedObjects::create();
        textureOptions->findFileCallback = [vfs](const vsg::Path &file, const vsg::Options *options) -> vsg::Path
        {
            for (auto &dir : options->paths)
            {
                auto corrected = Misc::ResourceHelpers::correctResourcePath(dir, file, vfs);
                if (corrected != file)
                    return corrected;
            }
            return file;
        };
        return textureOptions;
    }

    vsg::ref_ptr<const vsg::Options> createNodeOptions(const VFS::Manager *vfs, const Pipeline::Builder &builder, vsg::ref_ptr<const vsg::Options> textureOptions)
    {
        auto vfsReader = vsg::ref_ptr{new vsgAdapters::vfs(*vfs)};
        vfsReader->add(vsg::ref_ptr{new vsgAdapters::nif(builder, textureOptions)});
        auto nodeOptions = vsg::Options::create();
        nodeOptions->sharedObjects = vsg::SharedObjects::create();
        nodeOptions->findFileCallback = [](const vsg::Path &file, const vsg::Options *options) -> auto
        {
            return "meshes/" + file;
        };
        nodeOptions->readerWriters = {vfsReader};
        return nodeOptions;
    }

    vsg::ref_ptr<const vsg::Options> createAnimationOptions(const VFS::Manager *vfs)
    {
        auto vfsReader = vsg::ref_ptr{new vsgAdapters::vfs(*vfs)};
        vfsReader->add(vsg::ref_ptr{new vsgAdapters::kf});
        auto options = vsg::Options::create();
        options->findFileCallback = [](const vsg::Path &file, const vsg::Options *options) -> vsg::Path
        {
            auto f = file;
            auto ext = vsg::lowerCaseFileExtension(file);
            if (ext == ".nif")
                f.replace(f.size()-4, 4, ".kf");
            return std::string("meshes/") + f;
        };
        options->readerWriters = {vfsReader};
        options->sharedObjects = vsg::SharedObjects::create();
        return options;
    }
}

namespace Resource
{
//vsgopenmw-delete-me
class ImageManager {};

    ResourceSystem::ResourceSystem(const VFS::Manager *vfs, const std::string &shaderPath)
        : mVFS(vfs)
        , shaderOptions(createShaderOptions(shaderPath))
        , builder(new Pipeline::Builder(shaderOptions))
        , imageOptions(createImageOptions(vfs))
        , textureOptions(createTextureOptions(vfs, *imageOptions))
        , nodeOptions(createNodeOptions(vfs, *builder, textureOptions))
        , animationOptions(createAnimationOptions(vfs))
    {
        mNifFileManager = std::make_unique<NifFileManager>(vfs);
        addResourceManager(mNifFileManager.get());
    }

    ResourceSystem::~ResourceSystem()
    {
        // this has to be defined in the .cpp file as we can't delete incomplete types

        mResourceManagers.clear();
    }

    SceneManager* ResourceSystem::getSceneManager()
    {
        return nullptr;//mSceneManager.get();
    }

    ImageManager* ResourceSystem::getImageManager()
    {
        return nullptr;//mImageManager.get();
    }

    NifFileManager* ResourceSystem::getNifFileManager()
    {
        return mNifFileManager.get();
    }

    KeyframeManager* ResourceSystem::getKeyframeManager()
    {
        return nullptr;//mSceneManager.get();
    }

    void ResourceSystem::setExpiryDelay(double expiryDelay)
    {
    }

    void ResourceSystem::updateCache(double referenceTime)
    {
    }

    void ResourceSystem::clearCache()
    {
        for (std::vector<BaseResourceManager*>::iterator it = mResourceManagers.begin(); it != mResourceManagers.end(); ++it)
            (*it)->clearCache();
    }

    void ResourceSystem::addResourceManager(BaseResourceManager *resourceMgr)
    {
        mResourceManagers.push_back(resourceMgr);
    }

    void ResourceSystem::removeResourceManager(BaseResourceManager *resourceMgr)
    {
        std::vector<BaseResourceManager*>::iterator found = std::find(mResourceManagers.begin(), mResourceManagers.end(), resourceMgr);
        if (found != mResourceManagers.end())
            mResourceManagers.erase(found);
    }

    const VFS::Manager* ResourceSystem::getVFS() const
    {
        return mVFS;
    }

    void ResourceSystem::reportStats(unsigned int frameNumber, osg::Stats *stats) const
    {
        for (std::vector<BaseResourceManager*>::const_iterator it = mResourceManagers.begin(); it != mResourceManagers.end(); ++it)
            (*it)->reportStats(frameNumber, stats);
    }

    void ResourceSystem::releaseGLObjects(osg::State *state)
    {
        for (std::vector<BaseResourceManager*>::const_iterator it = mResourceManagers.begin(); it != mResourceManagers.end(); ++it)
            (*it)->releaseGLObjects(state);
    }
}
