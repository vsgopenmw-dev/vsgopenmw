#pragma import_defines(MATERIAL)

#include "lib/material/descriptors_frag.glsl"
#include "lib/view/scene.glsl"

layout(location=0) out vec4 outColor;

void main()
{
    vec3 forward = vec3(0,0,-1);
    vec3 sun = scene.lightViewPos.xyz;
    float angleRadians = acos(dot(forward, sun));
    float angleMaxRadians = material.emissive.a;
    float fade = 1.0 - min(1.0, angleRadians / angleMaxRadians);
    if (fade <= 0)
        discard;
    outColor = vec4(material.emissive.xyz, material.diffuse.a * fade);
}

