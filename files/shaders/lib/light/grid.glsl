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

vec3 viewPosToGridCoord(vec2 fragPos, float viewZ, vec2 tileSize, float near, float far)
{
    vec3 c;
    c.xy = fragPos / tileSize;
    c.z = min(float(gridSize.z - 1), max(0.f, float(gridSize.z) * log((-viewZ - near) / (far - near) + 1.f)));
    return c;
}
