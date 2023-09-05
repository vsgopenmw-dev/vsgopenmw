mat3 rotate(float angle, vec3 vec)
{
    float c = cos(angle);
    float s = sin(angle);
    vec3 v_one_minus_c = vec * (1 - c);
    vec3 vs = vec * s;
    return mat3(v_one_minus_c * vec.xxx + vec3(c,vs.z,-vs.y),
                v_one_minus_c * vec.yyy + vec3(-vs.z, c, vs.x),
                v_one_minus_c * vec.zzz + vec3(vs.y, -vs.x, c));
}
