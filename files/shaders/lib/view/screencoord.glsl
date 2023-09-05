#include "lib/math/projection.glsl"

/*
 * Applies perspective projections.
 */

vec2 clipPosToScreenCoord(vec4 clipPos)
{
    vec3 ndc = clipPos.xyz / clipPos.w;
    return ndc.xy * 0.5 + 0.5;
}

vec2 viewPosToScreenCoord(vec3 viewPos, mat4 projection)
{
    //if (isPerspectiveProjection)
    vec4 clipPos = mulPerspective(projection, vec4(viewPos, 1.0));
    //else clipPos = projection * vec4(viewPos, 1.f);
    return clipPosToScreenCoord(clipPos);
}

vec4 screenCoordToViewPos(vec2 screenCoord, float depth, mat4 invProjection)
{
    vec4 clipPos = vec4(screenCoord * 2.0 - 1.0, depth, 1.0);
    //vec4 viewPos = invProjection * clipPos;
    vec4 viewPos = mulInvPerspective(invProjection, clipPos);
    return viewPos / viewPos.w;
}
