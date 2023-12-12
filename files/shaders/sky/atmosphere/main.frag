#include "lib/view/scene.glsl"

layout(location=0) in Inputs
{
#include "inout.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
    outColor = vec4(mix(scene.fogColor.xyz, scene.skyColor.xyz, frag_in.fade), 1.0);
}

