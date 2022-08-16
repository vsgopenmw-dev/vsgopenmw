#pragma import_defines (COLOR, TEXCOORD, MATERIAL, DIFFUSE_MAP)

#include "lib/material/descriptors_frag.glsl"
#include "lib/view/scene.glsl"

layout(location=0) in Inputs
{
#include "inout.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
    vec4 base = texture(diffuseMap, frag_in.texCoord);
    outColor = vec4(base.xyz, frag_in.color.a);// * material.diffuse.a);
    outColor.xyz = clamp(outColor.xyz * (scene.fogColor.xyz + vec3(0.13, 0.13, 0.13)), 0.0, 1.0);
    // ease transition between clear color and atmosphere/clouds
    outColor = mix(vec4(scene.fogColor.xyz, outColor.a), outColor, frag_in.color.a);
}
