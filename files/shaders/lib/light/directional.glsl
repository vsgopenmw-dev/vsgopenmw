#include "equations.glsl"

vec3 directionalDiffuse(vec3 viewNormal)
{
    return scene.lightDiffuse.xyz * lambert(scene.lightViewPos.xyz, viewNormal);
}
