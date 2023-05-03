mat4 rotate(float angle, vec3 vec)
{
    float c = cos(angle);
    float s = sin(angle);
    float one_minus_c = 1 - c;
    float x = vec.x;
    float y = vec.y;
    float z = vec.z;
    return mat4(x * x * one_minus_c + c, y * x * one_minus_c + z * s, x * z * one_minus_c - y * s, 0,
                         x * y * one_minus_c - z * s, y * y * one_minus_c + c, y * z * one_minus_c + x * s, 0,
                         x * z * one_minus_c + y * s, y * z * one_minus_c - x * s, z * z * one_minus_c + c, 0,
                         0, 0, 0, 1);
}
