#include "lib/pushconstants.glsl"
#include "lib/view/scene.glsl"
#include "lib/view/screencoord.glsl"
#include "lib/light/pointdiffuse.glsl"

#include "bindings.glsl"
#include "data.glsl"
#include "attributes.glsl"
#
layout(location=VERTEX_LOCATION) in float inHeight;
layout(location=NORMAL_LOCATION) in vec3 inNormal;
layout(location=COLOR_LOCATION) in vec4 inColor;

layout(location=0) out Outputs
{
#include "inout.glsl"
} vert_out;

const float cellWorldSize = 8192;
const float cellVertices = 64;

layout(std140, set=TEXTURE_SET, binding=CHUNK_BINDING) uniform Data {
    Chunk chunk;
};

out gl_PerVertex{ vec4 gl_Position; };
void main()
{
    float numVerts = cellVertices * chunk.size + 1;
    int vertX = gl_VertexIndex / int(numVerts);
    int vertY = gl_VertexIndex % int(numVerts);
    float nX = vertX / (numVerts-1);
    float nY = vertY / (numVerts-1);
    /*
     * Calculates vertices relative to the camera position to avoid precision issues far from origin.
     */
    vec3 cameraPos = (scene.invView * vec4(0,0,0,1)).xyz;
    vec2 cameraCellPos = cameraPos.xy / cellWorldSize; // = scene.cameraCellCalculatedFromDoubleMatrix;
    vec2 offset = (chunk.center - cameraCellPos) * cellWorldSize;
    float scale = chunk.size * cellWorldSize;
    vec3 vertex = vec3(
        offset.x + (nX - 0.5f) * scale,
        offset.y + (nY - 0.5f) * scale,
        inHeight - cameraPos.z);
    mat4 invView = scene.invView;
    invView[3] = vec4(0, 0, 0, 1);
    /*
     * Doesn't use modelview matrix to avoid precision issues caused by different modelview matrices for neighboring chunks.
     */
    //vec4 viewPos = pc.modelview * vec4(vertex, 1.f);
    vec4 viewPos = vec4(vertex, 1.f) * invView; // openmw-6362-terrain-precision
    gl_Position = pc.projection * viewPos;
    vert_out.viewPos = viewPos.xyz;
    mat4 normalMatrix = inverse(transpose(pc.modelview));
    vert_out.viewNormal = (normalMatrix * vec4(inNormal,0.0)).xyz;
    vert_out.color = inColor;
    vert_out.texCoord = vec2(nX, nY);
    vert_out.pointLightDiffuse = pointLightDiffuse(clipPosToScreenCoord(gl_Position), normalize(vert_out.viewNormal), viewPos.xyz);
}
