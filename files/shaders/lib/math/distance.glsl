float distanceSquared(vec2 a, vec2 b)
{
    a -= b;
    return dot(a, a);
}

