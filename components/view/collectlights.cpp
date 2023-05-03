#include "collectlights.hpp"

namespace View
{
    CollectLights::CollectLights(size_t maxLights)
        : vsg::ViewDependentState(0)
    {
        mData = vsg::vec4Array::create(maxLights * 2); //vsgopenmw-fixme(dont-repeat-yourself(light-data-size))
        mData->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA_TRANSFER_AFTER_RECORD;
    }

    void CollectLights::pack()
    {
        auto light_itr = mData->begin();
        size_t maxLights = mData->size() / 2;
        size_t lightCount = std::min(maxLights, pointLights.size());
        for (size_t i = 0; i < lightCount; ++i)
        {
            auto& l = pointLights[i];
            auto& mv = l.first;
            auto& light = l.second;
            auto eye_position = vsg::vec3(mv * light->position);
            (*light_itr++).set(light->color.r, light->color.g, light->color.b, light->intensity);
            (*light_itr++).set(eye_position.x, eye_position.y, eye_position.z, 0);
        }
        mData->at(1).w = static_cast<float>(lightCount);
        if (maxLights < pointLights.size())
            ;//std::cerr << "maxLights < pointLights.size()" << std::endl;
        mData->dirty();
    }
}
