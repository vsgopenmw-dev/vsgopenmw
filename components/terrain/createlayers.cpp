#include "createlayers.hpp"

#include <cassert>

#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/DescriptorBuffer.h>

#include <components/pipeline/sets.hpp>
#include <components/pipeline/terrain.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/shareimage.hpp>

#include "defs.hpp"
#include "storage.hpp"

namespace Pipeline::Descriptors
{
    #include <files/shaders/terrain/bindings.glsl>
}
namespace Pipeline::Data
{
    using vsg::vec2;
    #include <files/shaders/terrain/data.glsl>
}
namespace Terrain
{
    CreateLayers::CreateLayers(
        vsg::ref_ptr<const vsg::Options> in_imageOptions, vsg::ref_ptr<const vsg::Options> in_shaderOptions, vsg::ref_ptr<vsg::Sampler> in_samplerOptions)
        : pipelineCache(std::make_unique<Pipeline::Terrain>(in_shaderOptions))
        , imageOptions(in_imageOptions)
        , shaderOptions(in_shaderOptions)
        , sharedObjects(vsg::SharedObjects::create())
        , samplerOptions(in_samplerOptions)
    {
    }

    CreateLayers::~CreateLayers() {}

    vsg::ref_ptr<vsg::StateGroup> CreateLayers::create(Storage& storage, float chunkSize, const vsg::vec2& chunkCenter) const
    {
        std::vector<LayerInfo> layerList;
        vsg::ref_ptr<vsg::Data> blendmaps;
        storage.getBlendmaps(chunkSize, vsg::vec2(chunkCenter.x, chunkCenter.y), blendmaps, layerList);

        auto bindPipeline = pipelineCache->getPipeline({ (uint32_t)layerList.size() });
        auto layout = bindPipeline->pipeline->layout;

        auto sampler = samplerOptions;
        vsg::Descriptors descriptors;

        blendmaps->properties.imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        descriptors.emplace_back(vsg::DescriptorImage::create(sampler, blendmaps, Pipeline::Descriptors::BLENDMAP_BINDING));

        vsg::ImageInfoList layerData;
        for (auto& layer : layerList)
        {
            auto data = vsgUtil::readImage(layer.mDiffuseMap, imageOptions);
            layerData.emplace_back(vsgUtil::sharedImageInfo(sharedObjects, sampler, data));
        }
        auto descriptor = vsg::DescriptorImage::create(layerData, Pipeline::Descriptors::LAYER_BINDING);
        vsgUtil::share_if(sharedObjects, descriptor);
        descriptors.push_back(descriptor);

        auto chunkData = vsg::Value<Pipeline::Data::Chunk>::create(Pipeline::Data::Chunk{ .center = chunkCenter, .size = chunkSize });
        auto chunkDescriptor = vsg::DescriptorBuffer::create(chunkData, Pipeline::Descriptors::CHUNK_BINDING);
        descriptors.push_back(chunkDescriptor);

        auto bds = vsg::BindDescriptorSet::create(
            VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::TEXTURE_SET, descriptors);
        auto sg = vsg::StateGroup::create();
        sg->stateCommands = { bindPipeline, bds };
        return sg;
    }
}
