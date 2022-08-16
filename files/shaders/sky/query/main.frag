#pragma import_defines(DIFFUSE_MAP)

#include "lib/material/descriptors_frag.glsl"

layout(location=0) in Inputs
{
#include "lib/inout_texcoord.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
    outColor = texture(diffuseMap, frag_in.texCoord);
    if (outColor.a < 0.5)
        discard;
}
