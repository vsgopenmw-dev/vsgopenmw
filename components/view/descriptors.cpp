#include "descriptors.hpp"

#include <vsg/state/DescriptorImage.h>
#include <vsg/state/DescriptorSet.h>
#include <vsg/state/BufferedDescriptorBuffer.h>
#include <vsg/maths/transform.h>
#include <vsg/viewer/Camera.h>

#include <components/pipeline/layout.hpp>
#include <components/render/depth.hpp>
#include <components/vsgutil/share.hpp>

#include "gridsize.hpp"

namespace
{
    #include <files/shaders/lib/light/maxlights.glsl>
    vsg::ref_ptr<vsg::uintArray> createLightGrid(int maxLights, int numClusters)
    {
        auto array = vsg::uintArray::create(numClusters*2);
        for (size_t i=0; i<array->size(); ++i)
            array->at(i) = (i%2==0) ? 0 : maxLights;
        return array;
    }
    vsg::ref_ptr<vsg::uintArray> createLightIndices(int maxLights, int numClusters)
    {
        int maxPer = std::min(maxLights, maxLightsPerCluster);
        auto array = vsg::uintArray::create(numClusters*maxPer);
        for (size_t i=0; i<array->size(); ++i)
            array->at(i) = i;
        return array;
    }
    vsg::ref_ptr<vsg::Descriptor> dummyEnvMap()
    {
        auto sampler = vsgUtil::shareDefault<vsg::Sampler>();
        auto image = vsg::ubvec4Array3D::create(1,1, Pipeline::Descriptors::VIEW_ENV_COUNT, vsg::ubvec4(0,0,0,0), vsg::Data::Layout{.format=VK_FORMAT_R8G8B8A8_UNORM, .imageViewType=VK_IMAGE_VIEW_TYPE_2D_ARRAY});
        return vsg::DescriptorImage::create(sampler, image, Pipeline::Descriptors::VIEW_ENV_BINDING, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
    vsg::ref_ptr<vsg::Descriptor> dummyShadowMap()
    {
        auto sampler = vsg::Sampler::create();
        sampler->minFilter = sampler->magFilter = VK_FILTER_NEAREST;
        auto image = vsg::floatArray3D::create(1,1,1, Render::farDepth, vsg::Data::Layout{.format=VK_FORMAT_D32_SFLOAT, .imageViewType=VK_IMAGE_VIEW_TYPE_2D_ARRAY});
        return vsg::DescriptorImage::create(sampler, image, Pipeline::Descriptors::VIEW_SHADOW_MAP_BINDING, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
}
namespace View
{
    Descriptors::Descriptors(int maxLights, vsg::ref_ptr<vsg::Descriptor> envMap, vsg::ref_ptr<vsg::Descriptor> shadowMap)
        : mScene(Pipeline::Descriptors::VIEW_SCENE_BINDING)
    {
        if (!envMap)
            envMap = vsgUtil::share<vsg::Descriptor>(dummyEnvMap);
        if (!shadowMap)
            shadowMap = vsgUtil::share<vsg::Descriptor>(dummyShadowMap);

        vsg::uivec3 gridSize = {gridSizeX, gridSizeY, gridSizeZ};
        if(maxLights<=1)
            gridSize={1,1,1};
        int numClusters = gridSize.x * gridSize.y * gridSize.z;

        maxLights = std::max(maxLights, 1);

        mLightData = vsg::vec4Array::create(maxLights*2);
        mLightDescriptor = vsg::BufferedDescriptorBuffer::create(mLightData, Pipeline::Descriptors::VIEW_LIGHTS_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
        auto size = vsg::DescriptorBuffer::create(vsg::uivec4Value::create(vsg::uivec4(gridSize, 0)), Pipeline::Descriptors::VIEW_LIGHT_GRID_SIZE_BINDING, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        auto lightGrid = vsg::DescriptorBuffer::create(createLightGrid(maxLights, numClusters), Pipeline::Descriptors::VIEW_LIGHT_GRID_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        auto lightIndices = vsg::DescriptorBuffer::create(createLightIndices(maxLights, numClusters), Pipeline::Descriptors::VIEW_LIGHT_INDICES_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        vsg::Descriptors descriptors = {mScene.descriptor(), mLightDescriptor, envMap, shadowMap, lightGrid, size, lightIndices};
        mDescriptorSet = vsg::DescriptorSet::create(Pipeline::getViewDescriptorSetLayout(), descriptors);
    }

    void Descriptors::copyDataListToBuffers()
    {
        mScene.copyDataListToBuffers();
    }

    void Descriptors::setLightPosition(const vsg::vec3 &pos, vsg::Camera &camera)
    {
        sceneData().lightViewPos = vsg::vec4(vsg::vec3(vsg::normalize(vsg::dvec3(pos) * vsg::inverse_3x3(camera.viewMatrix->transform()))), 0.0);
    }
}
