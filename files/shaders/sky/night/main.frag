#pragma import_defines (COLOR, TEXCOORD, DIFFUSE_MAP, MATERIAL)

#include "lib/material/descriptors_frag.glsl"
#include "lib/view/scene.glsl"

layout(location=0) in Inputs
{
#include "inout.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
#ifdef DIFFUSE_MAP
    outColor = texture(diffuseMap, frag_in.texCoord);
#else
    outColor = vec4(1.0,1.0,1.0,1.0);
#endif
    outColor.a *= material.diffuse.a * scene.skyColor.a;
#ifdef COLOR
    outColor.a *= frag_in.color.a;
#endif
}

