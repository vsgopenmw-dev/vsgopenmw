
vec4 getDiffuse(vec4 vertexColor)
{
    if (colorMode == 2)
        return vertexColor;
#ifdef MATERIAL
    return material.diffuse;
#else
    return vec4(1);
#endif
}
vec3 getAmbient(vec4 vertexColor)
{
    if (colorMode == 2)
        return vertexColor.xyz;
#ifdef MATERIAL
    return material.ambient.xyz;
#else
    return vec3(1);
#endif
}
vec3 getEmission(vec4 vertexColor)
{
    if (colorMode == 1)
        return vertexColor.xyz;
#ifdef MATERIAL
    return material.emissive.xyz;
#else
    return vec3(0);
#endif
}

vec4 applyMaterial(vec3 lightDiffuse, vec4 base, vec4 vertexColor /*, Material material, Scene scene, constexpr int colorMode*/)
{
    vec3 ambient = scene.ambient.xyz * getAmbient(vertexColor);
    vec4 materialDiffuse = getDiffuse(vertexColor);
    vec3 diffuse = lightDiffuse * materialDiffuse.xyz;
    return vec4(ambient + diffuse + getEmission(vertexColor), materialDiffuse.a) * base;
}
