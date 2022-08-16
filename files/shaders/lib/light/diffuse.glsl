#include "equations.glsl"
#include "select.glsl"
#include "pointlights.glsl"

layout (std430, set=VIEW_SET, binding=VIEW_LIGHT_INDICES_BINDING) readonly buffer LightIndexBuffer{
    uint lightIndexList[];
};

layout (std430, set=VIEW_SET, binding=VIEW_LIGHT_GRID_BINDING) readonly buffer LightGridBuffer{
    LightGrid lightGrid[];
};

vec3 pointLightDiffuse(vec2 fragPos, vec3 viewNormal, vec3 viewPos)
{
    vec3 diffuse = vec3(0);
#if 0
    int count = int(pointLights[0].position.w);
    for (int l=0; l<count; ++l)
    {
        PointLight light = pointLights[l];
#else
    uint tileIndex = selectTile(fragPos, viewPos.z);
    uint count = min(uint(pointLights[0].position.w), lightGrid[tileIndex].count);
    uint offset = lightGrid[tileIndex].offset;
    for (uint l=0; l<count; ++l)
    {
        PointLight light = pointLights[lightIndexList[offset+l]];
#endif
        vec4 lightColor = light.colorIntensity;
        vec3 lightVec = light.position.xyz - viewPos;
        float d = length(lightVec);
        if (d <= lightRange(light.colorIntensity.w))
            diffuse += lightColor.xyz * lambert(lightVec/d, viewNormal) * attenuate(lightColor.w, d);
    }
    return diffuse;
}
