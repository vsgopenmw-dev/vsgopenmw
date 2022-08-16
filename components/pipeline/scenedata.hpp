#ifndef VSGOPENMW_PIPELINE_SCENEDATA_H
#define VSGOPENMW_PIPELINE_SCENEDATA_H

#include <vsg/maths/mat4.h>

namespace Pipeline
{
    namespace Data
    {
        using vsg::vec4;
        using vsg::vec2;
        using vsg::mat4;
        #include <files/shaders/lib/view/data.glsl>
    }
    namespace Descriptors
    {
        #include <files/shaders/lib/view/bindings.glsl>
    }

    struct Scene : public Data::Scene
    {
        Scene()
        {
            time = 0;
            fogStart = fogScale = 0;
        }
        void setFogRange(float start, float end)
        {
            fogStart = start;
            fogScale = 1.f / std::max(0.001f, end-start);
        }
        void setDepthRange(float near, float far)
        {
            zNear = near;
            zFar = far;
        }
    };
}

#endif
