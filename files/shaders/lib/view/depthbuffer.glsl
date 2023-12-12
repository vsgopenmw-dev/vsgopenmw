#include "lib/math/projection.glsl"

/*
 *  Given a depth buffer value and inverse perspective projection matrix, computes the camera-space (negative) z value.
 */
float depthBufferValueToViewZ(float depth, mat4 invProjection)
{
    //assert(isPerspective);
    return invProjection[3][2] / (invProjection[2][3] * depth + invProjection[3][3]);
}

float reconstructViewZ(sampler2D zBuffer, ivec2 texel, vec2 zBufferSize, mat4 invProjection)
{
    //assert(isPerspective);
    float sampledDepth = texelFetch(zBuffer, texel, 0).r;
    return depthBufferValueToViewZ(sampledDepth, invProjection);
}
