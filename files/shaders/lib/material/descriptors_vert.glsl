#include "bindings.glsl"
#include "lib/sets.glsl"

#ifdef TEXMAT
layout(std140, set=TEXTURE_SET, binding=TEXMAT_BINDING) uniform TexMat {
    mat4 texmat;
};
#endif
