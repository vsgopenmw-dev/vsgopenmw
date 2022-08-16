#include "createlayers.hpp"

#include <cassert>

#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/utils/SharedObjects.h>

#include <components/pipeline/sets.hpp>
#include <components/pipeline/terrain.hpp>
#include <components/pipeline/builder.hpp>
#include <components/vsgutil/readimage.hpp>

#include "defs.hpp"
#include "storage.hpp"

namespace Terrain
{
    CreateLayers::CreateLayers(vsg::ref_ptr<const vsg::Options> in_imageOptions, vsg::ref_ptr<const vsg::Options> in_shaderOptions)
        : pipelineCache(std::make_unique<Pipeline::Terrain>(in_shaderOptions))
        , imageOptions(in_imageOptions)
        , shaderOptions(in_shaderOptions)
        , sharedObjects(vsg::SharedObjects::create())
    {
    }

    CreateLayers::~CreateLayers()
    {
    }

    vsg::ref_ptr<vsg::StateGroup> CreateLayers::create(Storage &storage, float chunkSize, const vsg::vec2 &chunkCenter)
    {
        std::vector<LayerInfo> layerList;
        vsg::DataList blendmaps;
        storage.getBlendmaps(chunkSize, vsg::vec2(chunkCenter.x, chunkCenter.y), blendmaps, layerList);

        auto bindPipeline = pipelineCache->getPipeline({(uint32_t)layerList.size()});
        auto layout = bindPipeline->pipeline->layout;

        assert(!layerList.empty() && layerList.size() == blendmaps.size());
        auto sampler = sharedObjects->shared_default<vsg::Sampler>();
        vsg::Descriptors descriptors;
        {
            vsg::ImageInfoList layerData;
            for (auto &layer : layerList)
            {
                layerData.emplace_back(vsg::ImageInfo::create(sampler, vsgUtil::readImage(layer.mDiffuseMap, imageOptions)));
                sharedObjects->share(layerData.back());
            }
            descriptors.emplace_back(vsg::DescriptorImage::create(layerData, 0));
            sharedObjects->share(descriptors.back());
        }
        {
            vsg::ImageInfoList blendImageInfoList;
            for (auto &data : blendmaps)
                blendImageInfoList.emplace_back(vsg::ImageInfo::create(sampler, data));
            descriptors.emplace_back(vsg::DescriptorImage::create(blendImageInfoList, 1));
        }
        /*
        {
            auto blendData = vsg::ubyteArray3D::create(blendmaps[0]->s(), blendmaps[0]->t(), blendmaps.size(), vsg::Data::Layout{.format=VK_FORMAT_R8_UNORM, .imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY});
            auto ptr = blendData->data();
            for (auto map : blendmaps)
            {
                for (int s=0;s<map->s(); ++s)
                {
                    for (int t=0;t<map->t();++t)
                    {
                        *(ptr++) = *reinterpret_cast<unsigned char*>(map->data(s,t,0));
                    }
                }
            }
            descriptors.emplace_back(vsg::DescriptorImage::create(vsg::Sampler::create(), blendData, 1));
        }
        */
        auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::TEXTURE_SET, descriptors);
        auto sg = vsg::StateGroup::create();
        sg->stateCommands = {bindPipeline, bds};
        return sg;
    }
}
