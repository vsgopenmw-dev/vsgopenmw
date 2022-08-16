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
        CreateLayers(vsg::ref_ptr<const vsg::Options> imageOptions, vsg::ref_ptr<const vsg::Options> shaderOptions);
        ~CreateLayers();
        vsg::ref_ptr<vsg::StateGroup> create(Storage &storage, float chunkSize, const vsg::vec2 &chunkCenter);

        std::unique_ptr<Pipeline::Terrain> pipelineCache;
        vsg::ref_ptr<const vsg::Options> imageOptions;
        vsg::ref_ptr<const vsg::Options> shaderOptions;
        vsg::ref_ptr<vsg::SharedObjects> sharedObjects;
    };
}

#endif
