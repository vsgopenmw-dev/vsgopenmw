const int colorMode = 2;

#include "lib/view/scene.glsl"
#include "lib/view/fog.glsl"
#include "lib/view/shadow.glsl"
#include "lib/light/directional.glsl"
#include "lib/material/terms.glsl"
#include "bindings.glsl"
#include "terrain.settings"

layout(set=TEXTURE_SET, binding=BLENDMAP_BINDING) uniform sampler2D blendMap;

layout(set=TEXTURE_SET, binding=LAYER_BINDING) uniform sampler2DArray diffuseMap;

layout(set=TEXTURE_SET, binding=COLORMAP_BINDING) uniform sampler2D colorMap;

layout(location=0) in Inputs
{
#include "inout.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
    float blendTexSize = textureSize(blendMap, 0).x;
    vec2 blendCoord = frag_in.texCoord * blendTexSize;
    ivec2 blendTexel = ivec2(blendCoord);
    //blendTexel = min(blendTexel, int(blendTexSize-1));
    vec2 blendPos = blendCoord - blendTexel;
    blendPos = clamp((blendPos-0.5)*2+0.5, 0.0, 1.0);
    vec4 blendCoeffs = vec4(blendPos, 1-blendPos);
    blendCoeffs = blendCoeffs.zxzx * blendCoeffs.wwyy;
    ivec4 textures = ivec4(texelFetch(blendMap, blendTexel, 0) * 255);
    vec2 diffuseTexCoord = frag_in.texCoord * blendTexSize;

    outColor = vec4(0,0,0,1);
    for (int i=0; i<4; ++i)
        outColor.xyz += texture(diffuseMap, vec3(diffuseTexCoord, textures[i])).xyz * blendCoeffs[i];

    vec3 diffuse = frag_in.pointLightDiffuse + directionalDiffuse(normalize(frag_in.viewNormal)) * shadow(frag_in.viewPos, frag_in.viewNormal);

    vec2 colorTexSize = textureSize(colorMap, 0);
    vec2 colorCoord = (frag_in.texCoord - 0.5) * colorTexSize / (colorTexSize+1) + 0.5;
    vec4 vertexColor = texture(colorMap, colorCoord);
    outColor *= materialTerms(diffuse, vertexColor);
    outColor.xyz = applyFog(outColor.xyz, length(frag_in.viewPos));

#ifdef DEBUG_SHADOW
    outColor = debugShadow(outColor);
#endif

#ifdef DEBUG_CHUNKS
    float b = 0.005;
    if (frag_in.texCoord.x < b || frag_in.texCoord.y < b || frag_in.texCoord.x > 1-b ||  frag_in.texCoord.y > 1-b)
        outColor.xyz = vec3(1,0,0);
    if (frag_in.localTexCoord.x < b || frag_in.localTexCoord.y < b || frag_in.localTexCoord.x > 1-b ||  frag_in.localTexCoord.y > 1-b)
        outColor.xyz = vec3(0,0,1);
#endif
}
