#ifndef VSGOPENMW_VIEW_DESCRIPTORS_H
#define VSGOPENMW_VIEW_DESCRIPTORS_H

#include <components/pipeline/scenedata.hpp>
#include <components/pipeline/descriptorvalue.hpp>

namespace View
{
    /*
     * Contains VIEW_SET descriptors.
     */
    class Descriptors
    {
        Pipeline::DynamicDescriptorValue<Pipeline::Scene> mScene;
        vsg::ref_ptr<vsg::BufferedDescriptorBuffer> mLightDescriptor;
        vsg::ref_ptr<vsg::vec4Array> mLightData;
        vsg::ref_ptr<vsg::DescriptorSet> mDescriptorSet;
    public:
        Descriptors(int maxLights=1, vsg::ref_ptr<vsg::Descriptor> envMap={}, vsg::ref_ptr<vsg::Descriptor> shadowMap={});

        Pipeline::Scene &sceneData() { return mScene.value(); }
        vsg::ref_ptr<vsg::vec4Array> lightData() { return mLightData; }
        vsg::ref_ptr<vsg::BufferedDescriptorBuffer> lightDescriptor() { return mLightDescriptor; }

        void copyDataListToBuffers();
        vsg::ref_ptr<vsg::DescriptorSet> getDescriptorSet() { return mDescriptorSet; };

       /*;*/ 
        void setLightPosition(const vsg::vec3 &pos, vsg::Camera &camera);
    };
}

#endif
