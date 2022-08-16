#pragma import_defines(DIFFUSE_MAP, MATERIAL)

#include "lib/material/descriptors_frag.glsl"

layout(location=0) in Inputs
{
#include "lib/inout_texcoord.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
    outColor = texture(diffuseMap, frag_in.texCoord);
    outColor.xyz *= material.emissive.xyz;
    outColor.a *= material.diffuse.a;
    if (outColor.a == 0)
        discard;
}

