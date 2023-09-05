#include "descriptors.hpp"

#include <vsg/state/DescriptorBuffer.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/DescriptorSet.h>

#include <components/pipeline/layout.hpp>
#include <components/pipeline/viewbindings.hpp>
#include <components/render/depth.hpp>
#include <components/vsgutil/share.hpp>

#include "gridsize.hpp"

namespace
{
    vsg::ref_ptr<vsg::uintArray> createLightGrid(int maxLights, int numClusters)
    {
        auto array = vsg::uintArray::create(numClusters * 2);
        for (size_t i = 0; i < array->size(); ++i)
            array->at(i) = (i % 2 == 0) ? 0 : maxLights;
        return array;
    }

    vsg::ref_ptr<vsg::uintArray> createLightIndices(int maxLights, int numClusters, bool setDefaults)
    {
        #include <files/shaders/lib/light/maxlights.glsl>
        int maxPer = std::min(maxLights, maxLightsPerCluster);
        auto array = vsg::uintArray::create(numClusters * maxPer);
        if (setDefaults)
        {
            for (size_t i = 0; i < array->size(); ++i)
                array->at(i) = i;
        }
        return array;
    }
}
namespace View
{
    vsg::ref_ptr<vsg::Descriptor> dummyEnvMap()
    {
        auto sampler = vsgUtil::shareDefault<vsg::Sampler>();
        auto image = vsg::ubvec4Array3D::create(1, 1, Pipeline::Descriptors::VIEW_ENV_COUNT, vsg::ubvec4(0, 0, 0, 0),
            vsg::Data::Properties(VK_FORMAT_R8G8B8A8_UNORM));
        image->properties.imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        return vsg::DescriptorImage::create(
            sampler, image, Pipeline::Descriptors::VIEW_ENV_BINDING, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    vsg::ref_ptr<vsg::Descriptor> dummyShadowMap()
    {
        auto image = vsg::floatArray3D::create(1, 1, 1, Render::farDepth, vsg::Data::Properties(VK_FORMAT_D32_SFLOAT));
        image->properties.imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        return vsg::DescriptorImage::create(vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(), image), Pipeline::Descriptors::VIEW_SHADOW_MAP_BINDING, 0,
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    }

    vsg::ref_ptr<vsg::Descriptor> dummyShadowSampler()
    {
        auto sampler = vsg::Sampler::create();
        sampler->minFilter = sampler->magFilter = VK_FILTER_NEAREST;
        return vsg::DescriptorImage::create(vsg::ImageInfo::create(sampler, vsg::ref_ptr<vsg::ImageView>()), Pipeline::Descriptors::VIEW_SHADOW_SAMPLER_BINDING, 0, VK_DESCRIPTOR_TYPE_SAMPLER);
    }

    vsg::ref_ptr<vsg::DescriptorSet> createViewDescriptorSet(const vsg::Descriptors& descriptors)
    {
        return vsg::DescriptorSet::create(Pipeline::getViewDescriptorSetLayout(), descriptors);
    }

    vsg::Descriptors createLightingDescriptors(vsg::ref_ptr<vsg::Data> lightData, bool compute)
    {
        size_t maxLights = std::max(1ul, lightData->valueCount()/2); //vsgopenmw-fixme(dont-repeat-yourself(light-data-size))
        vsg::uivec3 gridSize = { gridSizeX, gridSizeY, gridSizeZ };
        if (maxLights <= 1)
            gridSize = { 1, 1, 1 };
        int numClusters = gridSize.x * gridSize.y * gridSize.z;

        auto lightGrid = vsg::DescriptorBuffer::create(createLightGrid(maxLights, numClusters),
            Pipeline::Descriptors::VIEW_LIGHT_GRID_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        auto lightIndices = vsg::DescriptorBuffer::create(createLightIndices(maxLights, numClusters, !compute),
            Pipeline::Descriptors::VIEW_LIGHT_INDICES_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        auto lightDescriptor = vsg::DescriptorBuffer::create(
            lightData, Pipeline::Descriptors::VIEW_LIGHTS_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        auto gridSizeDescriptor = vsg::DescriptorBuffer::create(vsg::uivec4Value::create(vsg::uivec4(gridSize, 0)), Pipeline::Descriptors::VIEW_LIGHT_GRID_SIZE_BINDING, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        return vsg::Descriptors{ lightDescriptor, gridSizeDescriptor, lightGrid, lightIndices };
    }
}
