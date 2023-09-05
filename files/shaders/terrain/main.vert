#include "lib/pushconstants.glsl"
#include "lib/view/scene.glsl"
#include "lib/view/screencoord.glsl"
#include "lib/light/pointdiffuse.glsl"

#include "bindings.glsl"
#include "data.glsl"
#include "terrain.settings"

layout(set=TEXTURE_SET, binding=HEIGHTMAP_BINDING) uniform sampler2D heightMap;
layout(set=TEXTURE_SET, binding=NORMALMAP_BINDING) uniform sampler2D normalMap;

layout(location=0) out Outputs
{
#include "inout.glsl"
} vert_out;

const float cellWorldSize = 8192;

layout(std140, set=TEXTURE_SET, binding=BATCH_DATA_BINDING) uniform Data {
    Batch batch;
};

out gl_PerVertex{ vec4 gl_Position; };
void main()
{
    int lod = gl_InstanceIndex & 0xff;
    int shiftSize = (gl_InstanceIndex >> 8) & 0xff;
    int numVerts = int(batch.numVerts-1) / (1 << (shiftSize + lod)) + 1;
    ivec2 vert = ivec2(gl_VertexIndex / numVerts, gl_VertexIndex % numVerts);
    vec2 gridPos = vec2((gl_InstanceIndex >> 16)&0xff, (gl_InstanceIndex >> 24)&0xff);
    float scale = 1.0 / (1 << shiftSize);
    vec2 uv = scale * (gridPos + vert / float(numVerts-1));
    ivec2 texSize = textureSize(heightMap, 0);
    vec2 vertexCoord = (uv - 0.5) * texSize / (texSize+1) + 0.5;
    float height = texture(heightMap, vertexCoord).x;
    vec3 cameraPos = (inverse(pc.modelview) * vec4(0,0,0,1)).xyz;
    vec2 cameraCellPos = cameraPos.xy / cellWorldSize; // = scene.cameraCellCalculatedFromDoubleMatrix;
    vec2 offset = (batch.origin - cameraCellPos) * cellWorldSize;
    vec3 vertex = vec3(offset + uv * batch.vertexScale, height - cameraPos.z);
    vec4 viewPos = pc.modelview * vec4(vertex, 0.f);
    viewPos.w = 1;
    gl_Position = pc.projection * viewPos;

    vec3 normal = texture(normalMap, vertexCoord).xyz;
    vec3 viewNormal = normalize((inverse(transpose(pc.modelview)) * vec4(normal, 0)).xyz);
    vert_out.viewPos = viewPos.xyz;
    vert_out.texCoord = uv;
    #ifdef DEBUG_CHUNKS
    vert_out.localTexCoord = vert / float(numVerts-1);
    #endif
    vert_out.viewNormal = viewNormal;
    vert_out.pointLightDiffuse = pointLightDiffuse(clipPosToScreenCoord(gl_Position), viewNormal, viewPos.xyz);
}
