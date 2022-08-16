#ifdef VERTEX
const int VERTEX_LOCATION=0;
layout(location=VERTEX_LOCATION) in vec3 inVertex;
#else
const int VERTEX_LOCATION=-1;
#endif

#ifdef NORMAL
const int NORMAL_LOCATION=VERTEX_LOCATION+1;
layout(location=NORMAL_LOCATION) in vec3 inNormal;
#else
const int NORMAL_LOCATION=VERTEX_LOCATION;
#endif

#ifdef COLOR
const int COLOR_LOCATION=NORMAL_LOCATION+1;
layout(location=COLOR_LOCATION) in vec4 inColor;
#else
const int COLOR_LOCATION=NORMAL_LOCATION;
#endif

#ifdef SKIN
const int BONE_INDICES_LOCATION=COLOR_LOCATION+1;
layout(location=BONE_INDICES_LOCATION) in vec4 inBoneIndices;
const int BONE_WEIGHTS_LOCATION=BONE_INDICES_LOCATION+1;
layout(location=BONE_WEIGHTS_LOCATION) in vec4 inBoneWeights;
layout(std430, set=TEXTURE_SET, binding=BONE_BINDING) readonly buffer BoneMatrices {
    mat4 boneMatrices[];
};
#else
const int BONE_WEIGHTS_LOCATION=COLOR_LOCATION;
#endif

#ifdef TEXCOORD
const int TEXCOORD_LOCATION=BONE_WEIGHTS_LOCATION+1;
layout(location=TEXCOORD_LOCATION) in vec2 inTexCoord[numUvSets];
#endif
