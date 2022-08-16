vec2 clipPosToFragPos(vec4 clipPos/*, Scene scene*/)
{
    vec3 ndc = clipPos.xyz / clipPos.w;
    return 0.5 * (1.f + ndc.xy) * scene.resolution;
}

vec2 viewPosToFragPos(vec3 viewPos, mat4 projection/*, Scene scene*/)
{
    vec4 clipPos = projection * vec4(viewPos, 1.f);
    return clipPosToFragPos(clipPos);
}
