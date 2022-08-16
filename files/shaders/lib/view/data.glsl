struct Scene
{
    vec4 cascadeSplits;
    mat4 viewToCascadeProj[4];
    vec4 lightDiffuse;
    vec4 lightSpecular;
    vec4 lightViewPos;
    vec4 ambient;
    vec4 fogColor;
    vec2 resolution;
    float fogStart;
    float fogScale;
    float time;
    float zNear;
    float zFar;
};
