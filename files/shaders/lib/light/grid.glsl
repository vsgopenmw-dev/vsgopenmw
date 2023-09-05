struct LightGrid
{
    uint offset;
    uint count;
};

#include "gridsize.glsl"

uint gridCoordToIndex(uvec3 coord)
{
    return coord.x + gridSize.x * coord.y + (gridSize.x * gridSize.y) * coord.z;
}

vec3 screenToGridCoord(vec2 screenCoord, float viewZ, float near, float far)
{
    return vec3(screenCoord * vec2(gridSize.x, gridSize.y), min(float(gridSize.z - 1), max(0.f, float(gridSize.z) * log((-viewZ - near) / (far - near) + 1.f))));
}
