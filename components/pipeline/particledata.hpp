#ifndef VSGOPENMW_PIPELINE_PARTICLEDATA_H
#define VSGOPENMW_PIPELINE_PARTICLEDATA_H

#include <vsg/maths/mat4.h>
#include <vsg/maths/vec3.h>

namespace Pipeline
{
    namespace Data
    {
        using vsg::mat4;
        using vsg::vec3;
        using vsg::vec4;
        using uint = unsigned int;
#include <files/shaders/lib/particle/data.glsl>
    }
}

#endif
