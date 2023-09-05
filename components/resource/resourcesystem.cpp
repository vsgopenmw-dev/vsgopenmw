#include "resourcesystem.hpp"

#include <vsgXchange/images.h>
// #include <vsgXchange/models.h>
#include <vsg/io/Options.h>
#include <vsg/io/glsl.h>
#include <vsg/nodes/Node.h>
#include <vsg/state/Sampler.h>
#include <vsg/utils/SharedObjects.h>

#include <components/misc/resourcehelpers.hpp>
#include <components/pipeline/builder.hpp>
#include <components/vsgadapters/nif/kf.hpp>
#include <components/vsgadapters/nif/nif.hpp>
#include <components/vsgadapters/vfs.hpp>

#include "fallback.hpp"
#include "decompress.hpp"
#include "shadersettings.hpp"

namespace
{
    vsg::ref_ptr<const vsg::Options> createShaderOptions(const std::string& shaderPath)
    {
        auto options = vsg::Options::create();
        auto shaderSettings = vsg::ref_ptr{ new Resource::ShaderSettings };
        options->paths = { shaderPath };
        options->readerWriters = { shaderSettings, vsg::glsl::create() };
        options->sharedObjects = vsg::SharedObjects::create();
        return options;
    }

    vsg::ref_ptr<const vsg::Options> createImageOptions(const VFS::Manager* vfs, bool decompress)
    {
        auto options = vsg::Options::create();
        auto images = vsgXchange::images::create();
        vsg::ReaderWriters children;
        if (decompress)
        {
            auto wrapper = vsg::ref_ptr{ new Resource::Decompress() };
            wrapper->readerWriters = { images->readerWriters };
            children = { wrapper };
        }
        else
            children = images->readerWriters;

        options->readerWriters = { vsg::ref_ptr{ new vsgAdapters::vfs(*vfs, children) } };
        options->sharedObjects = vsg::SharedObjects::create();
        return options;
    }

    vsg::ref_ptr<const vsg::Options> createTextureOptions(const VFS::Manager* vfs, const vsg::Options& imageOptions)
    {
        auto textureOptions = vsg::Options::create(imageOptions);
        textureOptions->paths = { "textures" };
        textureOptions->sharedObjects = vsg::SharedObjects::create();
        textureOptions->findFileCallback = [vfs](const vsg::Path& file, const vsg::Options* options) -> vsg::Path {
            for (auto& dir : options->paths)
            {
                auto corrected = Misc::ResourceHelpers::correctResourcePath(dir.c_str(), file.c_str(), vfs);
                if (vsg::Path(corrected) != file)
                    return vsg::Path(corrected);
            }
            return file;
        };
        auto fallback = vsg::ref_ptr{ new Resource::Fallback };
        fallback->fallbackObject = vsg::ubvec4Array2D::create(
            1, 1, vsg::ubvec4(0xff, 0x00, 0xff, 0xff), vsg::Data::Properties{ VK_FORMAT_R8G8B8A8_UNORM });
        textureOptions->readerWriters.push_back(fallback);
        return textureOptions;
    }

    vsg::ref_ptr<const vsg::Options> createNodeOptions(
        const VFS::Manager* vfs, const Pipeline::Builder& builder, vsg::ref_ptr<const vsg::Options> textureOptions)
    {
        auto nodeOptions = vsg::Options::create();
        nodeOptions->sharedObjects = vsg::SharedObjects::create();
        nodeOptions->findFileCallback = [](const vsg::Path& file, const vsg::Options* options) -> vsg::Path
        {
            return vsg::Path("meshes/") + file;
        };

        auto fallback = vsg::ref_ptr{ new Resource::Fallback };
        static const char* const sMeshTypes[] = { "nif", "osg", "osgt", "osgb", "osgx", "osg2", "dae" };
        for (unsigned int i = 0; i < sizeof(sMeshTypes) / sizeof(sMeshTypes[0]); ++i)
            fallback->fallbackFiles.push_back("marker_error." + std::string(sMeshTypes[i]));
        fallback->fallbackObject = vsg::Node::create();

        auto vfsReader = vsg::ref_ptr{ new vsgAdapters::vfs(*vfs) };
        vfsReader->add(vsg::ref_ptr{ new vsgAdapters::nif(builder, textureOptions) });
        // vfsReader->add(vsgXchange::models::create());

        nodeOptions->readerWriters = { vfsReader, fallback };
        return nodeOptions;
    }

    vsg::ref_ptr<const vsg::Options> createAnimationOptions(const VFS::Manager* vfs)
    {
        auto vfsReader = vsg::ref_ptr{ new vsgAdapters::vfs(*vfs, { vsg::ref_ptr{ new vsgAdapters::kf } }) };
        auto options = vsg::Options::create();
        options->findFileCallback = [vfs](const vsg::Path& in_file, const vsg::Options* options) -> vsg::Path {
            auto file = Misc::ResourceHelpers::correctActorModelPath("meshes/" + in_file, vfs);
            std::string basename = file;
            size_t slashpos = file.find_last_of("\\/");
            if (slashpos != std::string::npos && slashpos + 1 < file.size())
                basename = file.substr(slashpos + 1);
            if (basename.empty() || (basename[0] != 'x' && basename[0] != 'X'))
                return {};

            auto ext = vsg::lowerCaseFileExtension(file);
            if (ext == ".nif")
                file.replace(file.size() - 4, 4, ".kf");
            return file;
        };
        options->readerWriters = { vfsReader };
        options->sharedObjects = vsg::SharedObjects::create();
        return options;
    }
}

namespace Resource
{
    class NifFileManager {}; // vsgopenmw-delete-me

    ResourceSystem::ResourceSystem(const VFS::Manager* vfs, const std::string& shaderPath,
        vsg::ref_ptr<vsg::Sampler> defaultSampler, bool supportsCompressedImages)
        : mVFS(vfs)
        , shaderOptions(createShaderOptions(shaderPath))
        , builder(new Pipeline::Builder(shaderOptions, defaultSampler))
        , imageOptions(createImageOptions(vfs, !supportsCompressedImages))
        , textureOptions(createTextureOptions(vfs, *imageOptions))
        , nodeOptions(createNodeOptions(vfs, *builder, textureOptions))
        , animationOptions(createAnimationOptions(vfs))
    {
    }

    ResourceSystem::~ResourceSystem()
    {
    }

    // vsgopenmw-delete-me
    NifFileManager* ResourceSystem::getNifFileManager()
    {
        return {};
    }

    void ResourceSystem::setExpiryDelay(double expiryDelay) {}

    void ResourceSystem::updateCache(double referenceTime)
    {
        std::vector<vsg::ref_ptr<const vsg::Options>> options = { imageOptions, textureOptions, nodeOptions, animationOptions };
        for (auto& o : options)
            if (auto& sharedObjects = o->sharedObjects)
                sharedObjects->prune();
    }

    void ResourceSystem::clearCache()
    {
    }

    void ResourceSystem::addResourceManager(BaseResourceManager* resourceMgr)
    {
    }

    void ResourceSystem::removeResourceManager(BaseResourceManager* resourceMgr)
    {
    }

    const VFS::Manager* ResourceSystem::getVFS() const
    {
        return mVFS;
    }

    void ResourceSystem::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
    }
}
