#include "data.glsl"

layout(std140, set=OBJECT_SET, binding=0) uniform ObjectData {
    Object object;
};
