vec4 makePlane(vec3 norm, vec3 point)
{
    return vec4(norm, -dot(norm, point));
}

float distanceToPlane(vec4 plane, vec3 point)
{
    return dot(plane, vec4(point, 1));
}
