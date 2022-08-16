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
    vec4 base = texture(diffuseMap, frag_in.texCoord);
#else
    vec4 base = vec4(1.0,1.0,1.0,1.0);
#endif
#ifdef COLOR
    base = mix(scene.fogColor, base, frag_in.color.a);
#endif
    outColor = base * material.diffuse.a;
}

