vec3 applyFog(/*Scene scene, */vec3 src, float depth)
{
    float fogValue = clamp((depth - scene.fogStart) * scene.fogScale, 0.0, 1.0);
    return mix(src, scene.fogColor.xyz, fogValue);
}
