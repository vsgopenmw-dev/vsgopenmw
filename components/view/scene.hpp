#ifndef VSGOPENMW_VIEW_SCENE_H
#define VSGOPENMW_VIEW_SCENE_H

#include <components/pipeline/viewdata.hpp>
#include <components/pipeline/descriptorvalue.hpp>

namespace View
{
    /*
     * Conveniently manages scene data assigned to VIEW_SET.
     */
    struct Scene : public Pipeline::DynamicDescriptorValue<Pipeline::Data::Scene>
    {
        Scene();
        void setFogRange(float start, float end)
        {
            value().fogStart = start;
            value().fogScale = 1.f / std::max(0.001f, end - start);
            dirty();
        }
        void setDepthRange(float near, float far)
        {
            value().zNear = near;
            value().zFar = far;
            dirty();
        }
        void setLightPosition(const vsg::vec3& pos, const vsg::mat4& invView);
    };
}

#endif
