#pragma import_defines (NORMAL, PARTICLE, COLOR, TEXCOORD, DIFFUSE_MAP, DARK_MAP, DETAIL_MAP, DECAL_MAP, GLOW_MAP, BUMP_MAP, ENV_MAP, MATERIAL )

#include "lib/material/descriptors_frag.glsl"

layout(constant_id=0) const int numUvSets=1;
layout(constant_id=1) const int alphaTestMode=0;
layout(constant_id=2) const int colorMode=0;
layout(constant_id=3) const bool specular=false;
layout(constant_id=START_UVSET_CONSTANTS+DIFFUSE_UNIT) const int diffuseUV=0;
layout(constant_id=START_UVSET_CONSTANTS+DARK_UNIT) const int darkUV=0;
layout(constant_id=START_UVSET_CONSTANTS+DETAIL_UNIT) const int detailUV=0;
layout(constant_id=START_UVSET_CONSTANTS+DECAL_UNIT) const int decalUV=0;
layout(constant_id=START_UVSET_CONSTANTS+GLOW_UNIT) const int glowUV=0;
layout(constant_id=START_UVSET_CONSTANTS+BUMP_UNIT) const int bumpUV=0;

#include "lib/view/scene.glsl"
#include "lib/view/env.glsl"
#include "lib/view/fog.glsl"
#include "lib/view/shadow.glsl"
#include "lib/object/object.glsl"
#include "lib/material/alphatest.glsl"
#include "lib/material/terms.glsl"
#include "lib/light/directional.glsl"

layout(location=0) in Inputs
{
#include "lib/inout.glsl"
} frag_in;

layout(location=0) out vec4 outColor;

void main()
{
#ifdef DIFFUSE_MAP
    outColor = texture(diffuseMap, frag_in.texCoord[diffuseUV].st);
#else
    outColor = vec4(1);
#endif
#ifdef DARK_MAP
    outColor *= texture(darkMap, frag_in.texCoord[darkUV]);
#endif
#ifdef DETAIL_MAP
    outColor.xyz *= texture(detailMap, frag_in.texCoord[detailUV]).xyz * 2;
#endif

#ifdef MATERIAL
    if (!alphaTest(outColor.a, material.alphaTestCutoff))
        discard;
#endif
    if (/*depthPass*/ scene.cascadeSplits[0] == 1)
    {
        if (outColor.a < 0.5)
            discard;
        return;
    }

#ifdef DECAL_MAP
    vec4 decalTex = texture(decalMap, frag_in.texCoord[decalUV]);
    outColor.xyz = mix(outColor.xyz, decalTex.xyz, decalTex.a /* * diffuseColor.a*/);
#endif

#ifdef COLOR
    vec4 vertexColor = frag_in.color;
#else
    vec4 vertexColor = vec4(1);
#endif

#ifdef NORMAL
    vec3 viewNormal = normalize(frag_in.viewNormal);
    vec3 diffuseLight = frag_in.pointLightDiffuse + directionalDiffuse(viewNormal) * shadow(frag_in.viewPos);
#else
    vec3 diffuseLight = vec3(0);
#endif
    outColor *= materialTerms(diffuseLight, vertexColor);

#ifdef NORMAL
    if (object.envColor.w != 0)
        outColor.xyz += texture(sceneEnv, vec3(frag_in.envUV, envIndex(scene.time))).xyz * object.envColor.xyz;
#endif
#ifdef ENV_MAP
    float envLuma = 1.0;
    vec2 envTexCoordGen = frag_in.envUV;
 #if defined(BUMP_MAP) && defined(TEXCOORD)
/*
    vec4 bumpTex = texture(bumpMap, frag_in.texCoord[bumpUV]);
    envTexCoordGen += bumpTex.rg * bumpMapMatrix;
    envLuma = clamp(bumpTex.b * envMapLumaBias.x + envMapLumaBias.y, 0.0, 1.0);
*/
 #endif

    outColor.xyz += texture(envMap, envTexCoordGen).xyz * envLuma;
#endif

#ifdef GLOW_MAP
    outColor.xyz += texture(glowMap, frag_in.texCoord[glowUV]).xyz;
#endif
    outColor.xyz = applyFog(outColor.xyz, length(frag_in.viewPos));

#ifdef DEBUG_SHADOW
    outColor = debugShadow(outColor);
#endif
}
