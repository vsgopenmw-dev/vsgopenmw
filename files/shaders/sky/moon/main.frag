#pragma import_defines(GLOW_MAP, DIFFUSE_MAP, MATERIAL)

#include "lib/material/descriptors_frag.glsl"

layout(location=0) in Inputs
{
#include "lib/inout_texcoord.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
    vec4 phase = texture(glowMap, frag_in.texCoord);
    vec4 mask = texture(diffuseMap, frag_in.texCoord);
    vec4 blendedLayer = phase * material.emissive;
    outColor = vec4(blendedLayer.xyz + material.ambient.xyz, material.ambient.a * material.diffuse.a * mask.a);
}

