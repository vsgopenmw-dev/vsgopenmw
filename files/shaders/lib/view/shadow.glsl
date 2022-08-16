#include "cascades.glsl"

layout(set=VIEW_SET, binding=VIEW_SHADOW_MAP_BINDING) uniform sampler2DArray shadowMap;

float textureProj(vec4 shadowCoord, uint cascadeIndex)
{
    const float bias = 0.005;

    //if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
    if ( shadowCoord.z > 0.0 && shadowCoord.z < 1.0 )
    {
        float dist = texture(shadowMap, vec3(shadowCoord.st /*+ offset*/, cascadeIndex)).r;
        //if (shadowCoord.w > 0 && dist < shadowCoord.z - bias)
        //vsgopenmw-reverse-depth
        if (shadowCoord.w > 0 && dist > shadowCoord.z + bias)
            return 0;
    }
    return 1;
}

float shadow(vec3 viewPos/*, Scene scene*/)
{
    float far = scene.cascadeSplits[maxCascades-1];
    float fadeStart = 0.9*far;
    float fade = clamp(((viewPos.z - fadeStart) / (far - fadeStart)), 0.0, 1.0);
    if (fade == 1)
        return 1;

    // Get cascade index for the current fragment's view position
    uint cascadeIndex = 0;
    for(uint i=0; i<maxCascades-1; ++i)
    {
        if(viewPos.z < scene.cascadeSplits[i] && scene.cascadeSplits[i] != 0)
            cascadeIndex = i + 1;
    }
    // Depth compare for shadowing
    vec4 shadowCoord = (scene.viewToCascadeProj[cascadeIndex]) * vec4(viewPos, 1.0);

    return mix(textureProj(shadowCoord / shadowCoord.w, cascadeIndex), 1, fade);
}

#ifdef DEBUG_SHADOW
vec4 debugShadow(vec4 color)
{
    uint width = uint(scene.resolution.x)/4;
    for(int i=0; i<maxCascades; ++i)
    {
        if (scene.cascadeSplits[i] != 0 && gl_FragCoord.x < width + width*i && gl_FragCoord.x > width*i && gl_FragCoord.y < width)
        {
            vec2 coord = (gl_FragCoord.xy-vec2(width*i,0))/width;
            float depth = texture(shadowMap, vec3(coord, i)).r;
            return vec4(vec3(depth),1);
        }
    }
    return color;
}
#endif
