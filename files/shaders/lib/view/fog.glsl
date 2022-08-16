
vec4 applyFog(/*Scene scene, */vec4 src, float depth)
{
    float fogValue = clamp((depth - scene.fogStart) * scene.fogScale, 0.0, 1.0);
    return vec4(mix(src.xyz, scene.fogColor.xyz, fogValue),src.a);
}

