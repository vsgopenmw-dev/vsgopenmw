#if defined(COLOR) || defined(PARTICLE)
vec4 color;
#endif
#if defined(TEXCOORD) || defined(PARTICLE)
vec2 texCoord[numUvSets];
#endif
vec3 viewPos;
#ifdef NORMAL
vec3 viewNormal;
 //ifdef ENV_MAP
 vec2 envUV;
 //endif
#endif
vec3 pointLightDiffuse;
