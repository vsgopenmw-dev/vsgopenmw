#include "cascades.glsl"

#include "shadow.settings"

layout(set=VIEW_SET, binding=VIEW_SHADOW_MAP_BINDING) uniform texture2DArray shadowMap;
layout(set=VIEW_SET, binding=VIEW_SHADOW_SAMPLER_BINDING) uniform sampler shadowSampler;

const vec2 poissonDisk[16] = {
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
};

float shadowPCF(vec4 shadowCoord, float uvFilterRadius, uint samples, uint cascadeIndex)
{
    float shadow = 0.0;
    for (uint i = 0; i < samples; ++i)
    {
        shadow += texture(sampler2DArrayShadow(shadowMap, shadowSampler), vec4(shadowCoord.st + uvFilterRadius * poissonDisk[i], cascadeIndex, shadowCoord.z)).r;
    }
    return shadow / samples;
}

// Based on "Percentage-Closer Soft Shadows", Randima Fernando, NVIDIA Corporation
float shadowPCSS_ortho_directional(vec4 shadowCoord, uint cascadeIndex)
{
    // Step 0 - configuration
    float lightFrustumSize = scene.cascadeFrustumWidths[cascadeIndex]; // Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT
    const float lightWorldSize = scene.lightViewPos.a;
    float uvLightSize = lightWorldSize / lightFrustumSize;
    float zReceiver = shadowCoord.z;
    // float searchWidth = uvLightSize * (zReceiverEyeSpace - near) / zReceiverEyeSpace;
    float searchWidth = uvLightSize; // Assuming that cascadeProj.near == 0

    // Step 1 - blocker search
    int blockers = 0;
    float avgBlockerDistance = 0;
    for (int i = 0; i < cBlockerSamples; ++i)
    {
        vec2 sampleCoord = poissonDisk[i] * searchWidth + shadowCoord.xy;
        float zBlocker = texture(sampler2DArray(shadowMap, shadowSampler), vec3(sampleCoord, cascadeIndex), 0).r;
        if (zBlocker > zReceiver) // vsgopenmw-reverse-depth
        {
            blockers++;
            avgBlockerDistance += zBlocker;
        }
    }
    if (blockers > 0)
        avgBlockerDistance /= blockers;
    else
        return 1;

    // Step 2 - penumbra estimation
    float penumbraWidth = (zReceiver - avgBlockerDistance) * uvLightSize / avgBlockerDistance;

    // Step 3 - percentage-closer filtering
    return shadowPCF(shadowCoord, penumbraWidth, cShadowSamples, cascadeIndex);
}

float shadow(vec3 viewPos, vec3 viewNormal/*, Scene scene*/)
{
    if (!cEnableShadows)
        return 1;
    float fade = 0;
    float far = scene.cascadeSplits[cShadowMapCount-1];
    if (far == 0) // if (!viewReceivesShadows)
        return 1;
    if (cShadowMaxDist != 0)
    {
        const float fadeStart = -cShadowFadeStart * cShadowMaxDist;
        fade = max(0.0, fadeStart - viewPos.z);
        if (cShadowFadeStart != 1)
            fade /= cShadowMaxDist + fadeStart;
        if (fade >= 1)
            return 1;
    }

    uint cascadeIndex = 0;
    for(uint i=0; i<cShadowMapCount-1; ++i)
    {
        if(viewPos.z < scene.cascadeSplits[i])
            cascadeIndex = i + 1;
    }
    vec4 shadowCoord = scene.viewToCascadeProj[cascadeIndex] * vec4(viewPos, 1.0);

    //if (shadowCoord.z > 0.0 && shadowCoord.z < 1.0 && shadowCoord.w > 0) //vk-depth-range
    {
        //shadowCoord /= shadowCoord.w;
        float NdotL = dot(viewNormal, -scene.lightViewPos.xyz);
        float bias = max(0.001 * (1.0 - NdotL), 0.0001);
        shadowCoord.z += bias;

        float shadow;
        if (cPCSS)
            shadow = shadowPCSS_ortho_directional(shadowCoord, cascadeIndex);
        else
            shadow = texture(sampler2DArrayShadow(shadowMap, shadowSampler), vec4(shadowCoord.st, cascadeIndex, shadowCoord.z)).r;
        return mix(shadow, 1, fade);
    }
    return 1;
}

#ifdef DEBUG_SHADOW
vec4 debugShadow(vec4 color)
{
    uint width = uint(scene.resolution.x)/4;
    for(int i=0; i<cShadowMapCount; ++i)
    {
        if (gl_FragCoord.x < width + width*i && gl_FragCoord.x > width*i && gl_FragCoord.y < width)
        {
            vec2 coord = (gl_FragCoord.xy-vec2(width*i,0))/width;
            float depth = texture(sampler2DArray(shadowMap, shadowSampler), vec3(coord, i), 0).r;
            return vec4(vec3(depth),1);
        }
    }
    return color;
}
#endif
