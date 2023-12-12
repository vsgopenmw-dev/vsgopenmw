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
    vec2 edge = vec2(1) + vec2(8) / scene.resolution;
    //vec2 edge = vec2(1.1);
    vec3 frustumCorners[8] = {
        vec3(edge.x, edge.y, nearDepth),
        vec3(-edge.x, edge.y, nearDepth),
        vec3(edge.x, -edge.y, nearDepth),
        vec3(-edge.x, -edge.y, nearDepth),
        vec3(edge.x, edge.y, farDepth),
        vec3(-edge.x, edge.y, farDepth),
        vec3(edge.x, -edge.y, farDepth),
        vec3(-edge.x, -edge.y, farDepth)
    };

    // Project frustum corners into local space
    mat4 invModelView = inverse(pc.modelview);
    mat4 inv = invModelView * scene.invProjection;
    for (int i=0; i<8; ++i)
    {
        vec4 invCorner = inv * vec4(frustumCorners[i], 1.0f);
        frustumCorners[i] = invCorner.xyz / invCorner.w;
    }

    float relativeCameraZ = invModelView[3].z;
    bool eyeUnderwater = relativeCameraZ < 0;
    int side = gl_VertexIndex % 2;
    vec3 farTop = frustumCorners[6+side];
    vec3 farBottom = frustumCorners[4+side];
    int underwaterSide = 2*int(eyeUnderwater);
    int underwaterSign = 1 - underwaterSide;

    // Prevent collisions with the near plane
    float bumpZ = scene.zNear * (1-clamp(abs(relativeCameraZ)/scene.zNear, 0, 1));
    float targetZ = bumpZ * -underwaterSign;

    vec3 vertex;
    if (underwaterSign * farTop.z >= 0 && underwaterSign * farBottom.z >= 0)
        vertex = farTop; //discard;
    else if (gl_VertexIndex > 1 && farTop.z > 0 && farBottom.z < 0)
    {
        // Horizon vertices
        vertex = slide(farTop, farBottom, targetZ);
    }
    else
    {
        // Screen edge vertices
        int index = (gl_VertexIndex + underwaterSide) % 4;
        vec3 near = frustumCorners[index];
        vec3 far = frustumCorners[4 + index];
        vertex = slide(near, far, targetZ);
    }

    vec4 viewPos = pc.modelview * vec4(vertex, 1.0);
    vert_out.viewPos = viewPos.xyz;
    gl_Position = pc.projection * viewPos;
}
