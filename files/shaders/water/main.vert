#include "lib/pushconstants.glsl"
#include "lib/attributes.glsl"
#include "lib/view/scene.glsl"
#include "lib/view/depthrange.glsl"

layout(location=0) out Outputs
{
#include "lib/inout.glsl"
} vert_out;

out gl_PerVertex{ vec4 gl_Position; };

vec3 slide(vec3 from, vec3 to, float targetZ)
{
    float zDiff = to.z-from.z;
    return mix(from, to, clamp(-(from.z-targetZ)/zDiff, 0, 1));
}

void main()
{
    const float one = 1.1;
    vec3 frustumCorners[8] = {
        vec3(one, one, nearDepth),
        vec3(-one, one, nearDepth),
        vec3(one, -one, nearDepth),
        vec3(-one, -one, nearDepth),
        vec3(one, one, farDepth),
        vec3(-one, one, farDepth),
        vec3(one, -one, farDepth),
        vec3(-one, -one, farDepth)
    };

    // Project frustum corners into local space
    mat4 inv = scene.invView * scene.invProjection;
    for (int i=0; i<8; ++i)
    {
        frustumCorners[i].xy *= 1.1;
        vec4 invCorner = inv * vec4(frustumCorners[i], 1.0f);
        frustumCorners[i] = invCorner.xyz / invCorner.w;
    }

    vec3 vertex;
    float relativeCameraZ = (inverse(pc.modelview) * vec4(0,0,0,1)).z;
    bool eyeUnderwater = relativeCameraZ < 0;
    int side = gl_VertexIndex % 2;
    vec3 top = frustumCorners[6+side];
    vec3 bottom = frustumCorners[4+side];
    int underwaterSide = 2*int(eyeUnderwater);
    int underwaterSign = 1 - underwaterSide;

    // Prevent collisions with the near plane
    float fadeThreshold = scene.zNear * (1-clamp(abs(relativeCameraZ)/scene.zNear, 0, 1));
    float bumpZ = fadeThreshold * -underwaterSign;

    if (underwaterSign * top.z >= 0 && underwaterSign * bottom.z >= 0)
        vertex = top; //discard;
    else if (gl_VertexIndex>1)
    {
        // Horizon vertices
        vertex = slide(top, bottom, 0);
    }
    else
    {
        // Screen edge vertices
        vec3 near = frustumCorners[gl_VertexIndex+underwaterSide];
        vec3 far = frustumCorners[4+gl_VertexIndex+underwaterSide];
        vertex = slide(near, far, bumpZ);
    }

    vec4 viewPos = pc.modelview * vec4(vertex, 1.0);
    vert_out.viewPos = viewPos.xyz;
    gl_Position = pc.projection * viewPos;
}
