layout(set=VIEW_SET, binding=VIEW_REFLECT_MAP_BINDING) uniform sampler2D reflectMap;

layout(set=VIEW_SET, binding=VIEW_REFLECT_DEPTH_MAP_BINDING) uniform sampler2D reflectDepthMap;

#include "raytrace.glsl"

float scaleThickness(float thickness, vec3 viewPos)
{
    /*
     * Based on how far away from the camera the depth is,
     * adding a bit of extra thickness can help improve some
     * artifacts. Driving this value up too high can cause
     * artifacts of its own.
     */
    const float factor = -1/2000;
    float depthScale = min(1.0f, viewPos.z * factor) * 5;
    return thickness + thickness * depthScale;
}

float scaleStride(float stride, vec3 viewPos)
{
    // As the distance grows further from the viewer and perspective projection makes objects smaller in screen space, the stride can be shortened and still likely find its contact point. This helps distant locations create higher quality reflections than if they were to use a large stride similar to closer locations.
    const float strideCutoff = -1/3000.0;
    stride *= 1.0 - min(1.0, viewPos.z * strideCutoff);
    return 1.0 + int(stride);
}

vec4 screenSpaceReflection(mat4 projection, mat4 invProjection, vec3 viewPos, vec3 viewNormal, float maxDistance)
{
    float thickness = scaleThickness(15, viewPos);
    float stride = scaleStride(10, viewPos);
    const float maxSteps = 25;
    float jitterFraction = float((int(gl_FragCoord.x+gl_FragCoord.y))&1)*0.5;

    vec3 viewVec = normalize(viewPos);
    vec3 pivot = normalize(reflect(viewVec, viewNormal));

    vec3 hitPointView;
    vec2 hitPixel;
    bool hit = traceScreenSpaceRay(viewPos.xyz, pivot, scene.projection, scene.invProjection, reflectDepthMap, thickness, -scene.zNear, stride, jitterFraction, maxSteps, maxDistance, hitPixel, hitPointView);
    return vec4(hitPixel / textureSize(reflectDepthMap, 0), 0, float(hit));
}

vec4 screenSpaceRefraction(mat4 projection, mat4 invProjection, vec3 viewPos, vec3 viewNormal, float ior, float maxDistance)
{
    float thickness = scaleThickness(15, viewPos);
    float stride = scaleStride(10, viewPos);
    const float maxSteps = 10;
    float jitterFraction = float((int(gl_FragCoord.x+gl_FragCoord.y))&1)*0.5;

    vec3 viewVec = normalize(viewPos);
    vec3 pivot = normalize(refract(viewVec, viewNormal, ior));

    vec3 hitPointView;
    vec2 hitPixel;
    bool hit = traceScreenSpaceRay(viewPos.xyz, pivot, scene.projection, scene.invProjection, reflectDepthMap, thickness, -scene.zNear, stride, jitterFraction, maxSteps, maxDistance, hitPixel, hitPointView);
    return vec4(hitPixel/textureSize(reflectDepthMap, 0), 0, float(hit));
}
