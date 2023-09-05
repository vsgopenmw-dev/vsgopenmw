#ifndef OPENMW_COMPONENTS_RESOURCE_RESOURCESYSTEM_H
#define OPENMW_COMPONENTS_RESOURCE_RESOURCESYSTEM_H

#include <vsg/core/ref_ptr.h>

#include <memory>
#include <vector>

namespace vsg
{
    class Options;
    class Sampler;
}

namespace VFS
{
    class Manager;
}

namespace osg
{
    class Stats;
}
namespace Pipeline
{
    class Builder;
}

namespace Resource
{
    class NifFileManager;
    class BaseResourceManager;

    /*
     * Assembles vsg::Options for reading files.
     */
    ///(
    /// @brief Wrapper class that constructs and provides access to the most commonly used resource subsystems.
    /// @par Resource subsystems can be used with multiple OpenGL contexts, just like the OSG equivalents, but
    ///     are built around the use of a single virtual file system.
    ///)
    class ResourceSystem
    {
    public:
        ResourceSystem(const VFS::Manager* vfs, const std::string& shaderPath,
            vsg::ref_ptr<vsg::Sampler> defaultSampler, bool supportsCompressedImages);
        ~ResourceSystem();

        NifFileManager* getNifFileManager();

        /// Indicates to each resource manager to clear the cache, i.e. to drop cached objects that are no longer
        /// referenced.
        /// @note May be called from any thread if you do not add or remove resource managers at that point.
        void updateCache(double referenceTime);

        /// Indicates to each resource manager to clear the entire cache.
        /// @note May be called from any thread if you do not add or remove resource managers at that point.
        void clearCache();

        /// Add this ResourceManager to be handled by the ResourceSystem.
        /// @note Does not transfer ownership.
        void addResourceManager(BaseResourceManager* resourceMgr);
        /// @note Do nothing if resourceMgr does not exist.
        /// @note Does not delete resourceMgr.
        void removeResourceManager(BaseResourceManager* resourceMgr);

        /// How long to keep objects in cache after no longer being referenced.
        void setExpiryDelay(double expiryDelay);

        /// @note May be called from any thread.
        const VFS::Manager* getVFS() const;

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const;

    private:
        std::unique_ptr<NifFileManager> mNifFileManager;

        // Store the base classes separately to get convenient access to the common interface
        // Here users can register their own resourcemanager as well
        std::vector<BaseResourceManager*> mResourceManagers;

        const VFS::Manager* mVFS;

        ResourceSystem(const ResourceSystem&);
        void operator=(const ResourceSystem&);

    public:
        const vsg::ref_ptr<const vsg::Options> shaderOptions;
        const std::unique_ptr<const Pipeline::Builder> builder;
        const vsg::ref_ptr<const vsg::Options> imageOptions;
        const vsg::ref_ptr<const vsg::Options> textureOptions;
        const vsg::ref_ptr<const vsg::Options> nodeOptions;
        const vsg::ref_ptr<const vsg::Options> animationOptions;
    };

}

#endif
