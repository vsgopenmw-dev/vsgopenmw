#include "lib/pushconstants.glsl"

layout(location=0) out Outputs
{
#include "lib/inout_texcoord.glsl"
} vert_out;

out gl_PerVertex{ vec4 gl_Position; };

void main()
{
    vert_out.texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2)*0.5;
    vec3 vertex = vec3(vert_out.texCoord-0.5, 0.0);
    gl_Position = pc.projection * (pc.modelview * vec4(vertex, 1.0));
    //vsgopenmw-reverse-depth
    gl_Position.z = 0.001; //farDepth+epsilon;
}
