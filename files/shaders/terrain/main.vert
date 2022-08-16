#include "lib/pushconstants.glsl"
#include "lib/sets.glsl"
#include "lib/view/scene.glsl"
#include "lib/view/fragpos.glsl"
#include "lib/light/diffuse.glsl"

#include "attributes.glsl"
layout(location= VERTEX_LOCATION) in vec3 inVertex;
layout(location= NORMAL_LOCATION) in vec3 inNormal;
layout(location= COLOR_LOCATION) in vec4 inColor;
layout(location= TEXCOORD_LOCATION) in vec2 inTexCoord;

layout(location=0) out Outputs
{
#include "inout.glsl"
} vert_out;

out gl_PerVertex{ vec4 gl_Position; };
void main()
{
    vec4 viewPos = pc.modelview * vec4(inVertex, 1.f);
    gl_Position = pc.projection * viewPos;
    vert_out.viewPos = viewPos.xyz;
    mat4 normalMatrix = inverse(transpose(pc.modelview));
    vert_out.viewNormal = (normalMatrix * vec4(inNormal,0.0)).xyz;
    vert_out.color = inColor;
    vert_out.texCoord = inTexCoord;
    vert_out.pointLightDiffuse = pointLightDiffuse(clipPosToFragPos(gl_Position), normalize(vert_out.viewNormal), viewPos.xyz);
}
