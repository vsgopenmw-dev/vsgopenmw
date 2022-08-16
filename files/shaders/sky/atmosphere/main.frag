#pragma import_defines (MATERIAL)

#include "lib/material/descriptors_frag.glsl"

layout(location=0) in Inputs
{
#include "inout.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
    outColor = mix(material.ambient, material.emissive, frag_in.fade);
}

