#include "lib/pushconstants.glsl"

layout(location = 0) in vec3 inVertex;

layout(location=0) out Outputs
{
#include "inout.glsl"
} vert_out;

out gl_PerVertex{ vec4 gl_Position; };

void main()
{
    gl_Position = pc.projection * (pc.modelview * vec4(inVertex, 1.0));

    // this is a cylinder, so every second vertex belongs to the bottom-most row
    if (gl_VertexIndex%2 == 1)
        vert_out.fade = 0;
    else
        vert_out.fade = 1;
}
