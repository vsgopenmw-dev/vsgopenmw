#pragma import_defines (VERTEX, NORMAL, COLOR, TEXCOORD, TEXMAT)

layout(constant_id=0) const int numUvSets=1;

#include "lib/material/descriptors_vert.glsl"
#include "lib/pushconstants.glsl"
#include "lib/attributes.glsl"

layout(location=0) out Outputs
{
#include "inout.glsl"
} vert_out;

out gl_PerVertex{ vec4 gl_Position; };

void main()
{
    gl_Position = pc.projection * (pc.modelview * vec4(inVertex, 1.0));
    if (gl_VertexIndex>= 49 && gl_VertexIndex <= 64)
        vert_out.color.a = 0.f; // bottom-most row
    else if (gl_VertexIndex >= 33 && gl_VertexIndex <= 48)
        vert_out.color.a = 0.25098; // second row
    else
        vert_out.color.a = 1.f;
    vert_out.texCoord = (texmat * vec4(inTexCoord[0], 0.0, 1.0)).xy;
}
