#pragma import_defines(BUMP_MAP)

#include "lib/material/descriptors_frag.glsl"
#include "lib/view/scene.glsl"
#include "lib/view/reflect.glsl"
#include "lib/view/screencoord.glsl"
#include "lib/view/shadow.glsl"
#include "lib/view/fog.glsl"
#include "lib/light/pointspecular.glsl"

#include "rainripples.glsl"
#include "fresnel.glsl"

layout(location=0) in Inputs
{
#include "lib/inout.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

const float shininess = 255;

const vec3 waterColorDeep = vec3(0.05,0.3,0.5);

const float waveChoppyness = 0.2;
const float waveScale = 175.0;

const vec2 bigWaves = vec2(0.2, 0.3);
const vec2 midWaves = vec2(0.9, 0.3);
const vec2 smallWaves = vec2(0.1, 0.1);

const float bump = 1.7;

const vec2 windDir = vec2(0.5f, -0.8f);
const float windSpeed = 0.2f;

vec2 normalCoords(vec2 uv, float scale, float speed, float time, float timer1, float timer2, vec3 previousNormal, vec2 dir)
{
    return uv * waveScale * scale + dir * time * windSpeed * speed -(previousNormal.xy/previousNormal.zz) * waveChoppyness + vec2(time * timer1, time * timer2);
}

vec3 getNormal(vec3 worldPos)
{
    vec2 uv = worldPos.xy / vec2(13653.0, -13653.0);
    vec2 dir = vec2(1.0,-1.0) * normalize(worldPos.xy + vec2(-20000.0,-69000.0));
    vec3 normal0 = 2.0 * texture(bumpMap, normalCoords(uv, 0.05, 0.04, scene.time, -0.015, -0.005, vec3(0.0,0.0,0.0), windDir)).rgb - 1.0;
    vec3 normal1a = 2.0 * texture(bumpMap, normalCoords(uv, 0.02,  0.05, 0.1 * scene.time, 0.0, 0.0, normal0, windDir * 1.0)).rgb - 1.0;
    vec3 normal1 = 2.0 * texture(bumpMap, normalCoords(uv, 0.001,  0.01, 0.0001 * scene.time, 0.0, 0.0, normal1a * 1.0, 0.1 * dir)).rgb - 1.0;
    vec3 normal2 = 2.0 * texture(bumpMap, normalCoords(uv, 0.25, 0.01, scene.time, -0.04, -0.03, normal1,windDir)).rgb - 1.0;
    vec3 normal3 = 2.0 * texture(bumpMap, normalCoords(uv, 0.5,  0.09, scene.time, 0.03, 0.04, normal2,windDir)).rgb - 1.0;

    vec3 rainRipple = vec3(0);
    if (scene.rainIntensity > 0.01)
        rainRipple = rainCombined(worldPos.xy/1000.0, scene.time) * scene.rainIntensity * 10;

    vec3 normal = normal1 * bigWaves.y + normal2 * midWaves.x  +  normal3 * midWaves.y + rainRipple;
    normal = normalize(vec3(-normal.x, -normal.y, normal.z * bump));
    return normal;
}

vec3 ambientReflection(vec3 worldDir)
{
    float fraction = worldDir.z > 0 ? clamp(worldDir.z*2-0.15, 0, 1) : 1-clamp(-worldDir.z*2-0.15,0.35,1);
    return mix(scene.fogColor.xyz, scene.skyColor.xyz, fraction);
}

vec3 colorRefraction(float waterDepth, float surfaceDepth, vec3 refraction)
{
    vec3 incidentLight = scene.lightDiffuse.xyz + scene.ambient.xyz;
    float inverseScatterAmount = exp(-0.00045 * waterDepth);
    vec3 deepColor = incidentLight * waterColorDeep * 0.25;
    deepColor = applyFog(deepColor, surfaceDepth);
    return mix(deepColor, refraction, clamp(inverseScatterAmount, 0.0, 1.0));
}

void main()
{
    vec3 worldPos = (scene.invView * vec4(frag_in.viewPos,1)).xyz;

    vec3 normal = getNormal(worldPos);
    vec3 viewNormal = (vec4(normal,0) * scene.invView).xyz;
    float viewLength = length(frag_in.viewPos);
    vec3 viewVec = frag_in.viewPos/viewLength;

    vec3 reflectedDir = reflect(viewVec, viewNormal);
    vec3 reflectedWorldDir = (scene.invView * vec4(reflectedDir, 0)).xyz;
    bool eyeUnderwater = reflectedWorldDir.z < 0;
    vec3 specular = pow(max(dot(reflectedDir, scene.lightViewPos.xyz), 0.0), shininess) * vec3(1,1,1);//*scene.lightSpecular.xyz;

    float ior = eyeUnderwater ? 0.75 : 1.333; // water to air ; air to water
    float fresnel = fresnel_dielectric(viewVec, viewNormal, ior);

    vec3 reflection;
    vec3 fallbackReflection = ambientReflection(reflectedWorldDir);
    vec4 reflected = screenSpaceReflection(scene.projection, scene.invProjection, frag_in.viewPos, viewNormal, scene.zFar);
    reflection = mix(fallbackReflection, texture(reflectMap, reflected.xy).xyz, reflected.w);

    vec2 screenCoord = gl_FragCoord.xy / scene.resolution;
    if (fresnel < 1)
    {
        vec4 refracted = screenSpaceRefraction(scene.projection, scene.invProjection, frag_in.viewPos, viewNormal, 1/ior, 2000);
        refracted.xy = mix(screenCoord, refracted.xy, refracted.w);

        float refractDepth = texture(reflectDepthMap, refracted.xy).r;
        vec4 refractedPos = screenCoordToViewPos(refracted.xy, refractDepth, scene.invProjection);
        vec3 refraction = texture(reflectMap, refracted.xy).xyz;

        float waterDepth = length(frag_in.viewPos - refractedPos.xyz);
        float surfaceDepth = viewLength;
        if (eyeUnderwater)
        {
            surfaceDepth = waterDepth;
            waterDepth = viewLength;
        }
        refraction.xyz = colorRefraction(waterDepth, viewLength, refraction.xyz);
        outColor = vec4(mix(refraction, reflection, fresnel), 1.0);
    }
    else
        outColor = vec4(reflection, 1.0);

    outColor.xyz += specular * shadow(frag_in.viewPos);
    vec3 pointSpec = pointLightSpecular(screenCoord, reflectedDir, frag_in.viewPos, shininess);
    pointSpec *= clamp(1-pointSpec.b*2.5, 0,1); // don't reflect ambient lights
    outColor.xyz += pointSpec;
}

