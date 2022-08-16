#pragma import_defines (VERTEX, NORMAL, COLOR, SKIN, TEXCOORD)

layout(constant_id=0) const int numUvSets=1;

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
#ifdef COLOR
    vert_out.color = inColor;
    vert_out.color = vec4(1,1,1,float(int(vert_out.color.x)));
#endif
#ifdef TEXCOORD
    vert_out.texCoord = inTexCoord[0];
#endif
}
