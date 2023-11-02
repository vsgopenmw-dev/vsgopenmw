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
    /*
     * Assembles vsg::Options for reading files.
     */
    class ResourceSystem
    {
    public:
        ResourceSystem(const VFS::Manager* vfs, const std::string& shaderPath,
            vsg::ref_ptr<vsg::Sampler> defaultSampler, bool supportsCompressedImages, int computeBin, int depthSortedBin);
        ~ResourceSystem();

        /// Indicates to each resource manager to clear the cache, i.e. to drop cached objects that are no longer
        /// referenced.
        /// @note May be called from any thread if you do not add or remove resource managers at that point.
        void updateCache(double referenceTime);

        /// Indicates to each resource manager to clear the entire cache.
        /// @note May be called from any thread if you do not add or remove resource managers at that point.
        void clearCache();

        /// How long to keep objects in cache after no longer being referenced.
        void setExpiryDelay(double expiryDelay);

        /// @note May be called from any thread.
        const VFS::Manager* getVFS() const;

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const;

    private:
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
