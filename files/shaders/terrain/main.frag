layout(constant_id = 0) const int layerCount = 1;

const int colorMode = 2;

#include "lib/view/scene.glsl"
#include "lib/view/fog.glsl"
#include "lib/view/shadow.glsl"
#include "lib/light/directional.glsl"
#include "lib/material/terms.glsl"
#include "bindings.glsl"

layout(set=TEXTURE_SET, binding=BLENDMAP_BINDING) uniform sampler2DArray blendMaps;

//uniform sampler2DArray diffuseMap; assert(equal_texture_sizes(diffuseMap))
layout(set=TEXTURE_SET, binding=LAYER_BINDING) uniform sampler2D diffuseMap[layerCount];

layout(location=0) in Inputs
{
#include "inout.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
    //float blendTexSize = textureSize(blendMap[0], 0).x;
    float blendTexSize = textureSize(blendMaps, 0).x / 2; // vsgopenmw-fixme
    //vec2 blendTexCoord = frag_in.texCoord;
    vec2 blendTexCoord = (frag_in.texCoord + vec2(1.0/blendTexSize/4.0) - vec2(0.5)) * (blendTexSize/(blendTexSize+1.0)) + vec2(0.5);
    vec2 diffuseTexCoord = frag_in.texCoord * blendTexSize;
    outColor = vec4(0,0,0,1);
    for (int i=0; i<layerCount; ++i)
    {
        outColor += vec4(texture(diffuseMap[i],diffuseTexCoord).xyz
            * texture(blendMaps, vec3(blendTexCoord, i)).x, 0.0); // vsgopenmw-move-me("main.vert")
    }

    vec3 diffuse = frag_in.pointLightDiffuse + directionalDiffuse(normalize(frag_in.viewNormal)) * shadow(frag_in.viewPos);
    outColor *= materialTerms(diffuse, frag_in.color);
    outColor.xyz = applyFog(outColor.xyz, length(frag_in.viewPos));
#ifdef DEBUG_SHADOW
    outColor = debugShadow(outColor);
#endif
}
