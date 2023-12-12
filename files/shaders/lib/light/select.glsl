#include "grid.glsl"

layout (std430, set=VIEW_SET, binding=VIEW_LIGHT_INDICES_BINDING) readonly buffer LightIndexBuffer{
    uint lightIndexList[];
};

layout (std430, set=VIEW_SET, binding=VIEW_LIGHT_GRID_BINDING) readonly buffer LightGridBuffer{
    LightGrid lightGrid[];
};

uint selectTile(vec2 screenCoord, float viewZ)
{
    uvec3 coord = uvec3(screenToGridCoord(screenCoord, viewZ, scene.zNear, scene.zFar));
    return gridCoordToIndex(coord);
}
