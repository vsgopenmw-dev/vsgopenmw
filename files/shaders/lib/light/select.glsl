#include "grid.glsl"

uint selectTile(vec2 fragPos, float viewZ)
{
    float zNear = scene.zNear;
    float zFar = scene.zFar;
    vec2 tileSize = scene.resolution / vec2(gridSize.x, gridSize.y);
    uvec3 coord = uvec3(viewPosToGridCoord(fragPos, viewZ, tileSize, scene.zNear, scene.zFar));
    return gridCoordToIndex(coord);
}
