#include "equations.glsl"
#include "select.glsl"
#include "pointlights.glsl"

vec3 pointLightSpecular(vec2 screenCoord, vec3 reflectedViewNormal, vec3 viewPos, float shininess)
{
    vec3 specular = vec3(0);
    uint tileIndex = selectTile(screenCoord, viewPos.z);
    uint count = min(uint(pointLights[0].position.w), lightGrid[tileIndex].count);
    uint offset = lightGrid[tileIndex].offset;
    for (uint l=0; l<count; ++l)
    {
        PointLight light = pointLights[lightIndexList[offset+l]];
        vec4 lightColor = light.colorIntensity;
        vec3 lightViewPos = light.position.xyz;
        float d = length(light.position.xyz - viewPos.xyz);
        float range = lightRange(light.colorIntensity.w);
        if (d <= range)
            specular += pow(max(dot(reflectedViewNormal, normalize(lightViewPos)), 0.0), shininess) * lightColor.xyz * attenuate(lightColor.w, d);
    }
    return specular;
}
