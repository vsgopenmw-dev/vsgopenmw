#ifndef VSGOPENMW_TERRAIN_CREATELAYERS_H
#define VSGOPENMW_TERRAIN_CREATELAYERS_H

#include <vsg/nodes/StateGroup.h>

#include <components/pipeline/terrain.hpp>

namespace Terrain
{
    class Storage;

    class CreateLayers
    {
    public:
        CreateLayers(vsg::ref_ptr<const vsg::Options> in_imageOptions, vsg::ref_ptr<const vsg::Options> in_shaderOptions, vsg::ref_ptr<vsg::Sampler> in_samplerOptions);
        ~CreateLayers();
        vsg::ref_ptr<vsg::StateGroup> create(Storage& storage, float chunkSize, const vsg::vec2& chunkCenter) const;

        std::unique_ptr<Pipeline::Terrain> pipelineCache;
        vsg::ref_ptr<const vsg::Options> imageOptions;
        vsg::ref_ptr<const vsg::Options> shaderOptions;
        vsg::ref_ptr<vsg::SharedObjects> sharedObjects;
        vsg::ref_ptr<vsg::Sampler> samplerOptions;
    };
}

#endif
