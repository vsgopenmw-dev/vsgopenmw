#include "collectlights.hpp"

#include <iostream>

namespace View
{
    CollectLights::CollectLights(vsg::ref_ptr<vsg::BufferedDescriptorBuffer> descriptor, vsg::ref_ptr<vsg::vec4Array> data)
        : vsg::ViewDependentState(0)
    {
        mLightData = data;
        //lightDescriptor = descriptor;
        mDescriptor = descriptor;
    }

    void CollectLights::pack()
    {
        auto light_itr = mLightData->begin();
        size_t maxLights = mLightData->size()/2;
        size_t lightCount = std::min(maxLights, pointLights.size());
        for (size_t i=0; i<lightCount; ++i)
        {
            auto &l = pointLights[i];
            auto &mv = l.first;
            auto &light = l.second;
            auto eye_position = vsg::vec3(mv * light->position);
            (*light_itr++).set(light->color.r, light->color.g, light->color.b, light->intensity);
            (*light_itr++).set(eye_position.x, eye_position.y, eye_position.z, 0);
        }
        mLightData->at(1).w = static_cast<float>(lightCount);
        if (maxLights < pointLights.size())
            std::cout << "maxLights < pointLights.size()" << std::endl;
    }

    void CollectLights::copy()
    {
        mDescriptor->vsg::DescriptorBuffer::copyDataListToBuffers();
    }
}
