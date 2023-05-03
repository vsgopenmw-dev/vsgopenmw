#include "equations.glsl"
#include "select.glsl"
#include "pointlights.glsl"

vec3 pointLightDiffuse(vec2 screenCoord, vec3 viewNormal, vec3 viewPos)
{
    vec3 diffuse = vec3(0);
    uint tileIndex = selectTile(screenCoord, viewPos.z);
    uint count = min(uint(pointLights[0].position.w), lightGrid[tileIndex].count);
    uint offset = lightGrid[tileIndex].offset;
    for (uint l=0; l<count; ++l)
    {
        PointLight light = pointLights[lightIndexList[offset+l]];
        vec4 lightColor = light.colorIntensity;
        vec3 lightVec = light.position.xyz - viewPos;
        float d = length(lightVec);
        if (d <= lightRange(light.colorIntensity.w))
            diffuse += lightColor.xyz * lambert(lightVec/d, viewNormal) * attenuate(lightColor.w, d);
    }
    return diffuse;
}
