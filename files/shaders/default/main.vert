#pragma import_defines (PARTICLE, BILLBOARD, MORPH, SKIN, TEXMAT, VERTEX, NORMAL, COLOR, TEXCOORD, ENV_MAP)

#include "lib/pushconstants.glsl"
#include "lib/material/descriptors_vert.glsl"
#include "lib/view/scene.glsl"
#include "lib/view/fragpos.glsl"
#include "lib/light/diffuse.glsl"

layout(constant_id=0) const int numUvSets=1;
layout(constant_id=START_UVSET_CONSTANTS+TEXMAT_BINDING) const int texmatUv=0;

#include "lib/attributes.glsl"

#ifdef MORPH
layout(std430, set=TEXTURE_SET, binding=MORPH_BINDING) readonly buffer Morph {
    vec4 morphdata[];
};
layout(std430, set=TEXTURE_SET, binding=MORPH_WEIGHTS_BINDING) readonly buffer MorphWeights {
    float morphweights[];
};
#endif

#ifdef PARTICLE
#include "lib/particle/data.glsl"
#include "lib/particle/util.glsl"
layout(std430, set=TEXTURE_SET, binding=PARTICLE_BINDING) readonly buffer ParticleBuffer{
    Particle particles[];
};
#endif

#ifdef BILLBOARD
#include "lib/math/billboard.glsl"
#endif

layout(location=0) out Outputs
{
#include "lib/inout.glsl"
} vert_out;

out gl_PerVertex{ vec4 gl_Position; };

void main()
{
#ifdef MORPH
    vec4 vertex = vec4(0,0,0,1);
    uint numMorphs = uint(morphdata[0].w);
    for (int i=0; i<numMorphs; ++i)
        vertex.xyz += morphweights[i] * morphdata[gl_VertexIndex*numMorphs+i].xyz;
#elif defined(PARTICLE)
    Particle particle = particles[gl_VertexIndex/4];
    int vertexIndex = gl_VertexIndex%4;
    vec2 texCoord = vec2((vertexIndex << 1) & 2, vertexIndex & 2);
    vec4 vertex = vec4(particle.positionSize.xyz, 1.0);
    vert_out.texCoord[0] = texCoord;
    vert_out.color = particle.color;
#elif defined(VERTEX)
    vec4 vertex = vec4(inVertex, 1.0);
#endif

#ifdef SKIN
    mat4 skinMat =
        inBoneWeights.x * boneMatrices[int(inBoneIndices.x)] +
        inBoneWeights.y * boneMatrices[int(inBoneIndices.y)] +
        inBoneWeights.z * boneMatrices[int(inBoneIndices.z)] +
        inBoneWeights.w * boneMatrices[int(inBoneIndices.w)];
    vertex = skinMat * vertex;
 #ifdef NORMAL
    vec3 normal = (vec4(inNormal,0) * inverse(skinMat)).xyz;
 #endif
#elif defined(NORMAL)
    vec3 normal = inNormal;
#endif

    vec4 viewPos =
#ifdef BILLBOARD
    billboardModelView(pc.modelview)
#else
    pc.modelview
#endif
     * vertex;
    vert_out.viewPos = viewPos.xyz;
    gl_Position = pc.projection * viewPos;

#ifdef PARTICLE
    if (!isDead(particle))
        gl_Position += vec4((texCoord - 0.5) * particle.positionSize.w, 0.0, 0.0);
#endif
#ifdef TEXCOORD
    for (int i=0; i<numUvSets;++i)
        vert_out.texCoord[i] = inTexCoord[i];
#endif
#ifdef TEXMAT
    vert_out.texCoord[texmatUv] = (texmat * vec4(vert_out.texCoord[texmatUv], 0.0, 1.0)).xy;
#endif
#ifdef NORMAL
    vec3 viewNormal = (vec4(normal,0) * inverse(pc.modelview)).xyz;
    vert_out.viewNormal = viewNormal;
 //ifdef ENV_MAP
    vec3 viewVec = normalize(viewPos.xyz);
    vec3 r = reflect( viewVec, viewNormal );
    float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
    vert_out.envUV = vec2(r.x/m + 0.5, r.y/m + 0.5);
 //endif
    vert_out.pointLightDiffuse = pointLightDiffuse(clipPosToFragPos(gl_Position), normalize(viewNormal), viewPos.xyz);
#endif
#ifdef COLOR
    vert_out.color = inColor;
#endif
}
