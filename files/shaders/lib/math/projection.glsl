#ifndef LIB_MATH_PROJECTION
#define LIB_MATH_PROJECTION

/*
 * Skips identity matrix components.
 */
vec4 mulInvPerspective(mat4 lhs, vec4 rhs)
{
    //assert(rhs.w == 1);
    return vec4(lhs[0][0] * rhs[0],
                lhs[1][1] * rhs[1],
                lhs[3][2],
                lhs[2][3] * rhs[2] + lhs[3][3]);
}
vec4 mulPerspective(mat4 lhs, vec4 rhs)
{
    //assert(rhs.w == 1);
    return vec4(lhs[0][0] * rhs[0],
                lhs[1][1] * rhs[1],
                lhs[2][2] * rhs[2] + lhs[3][2],
                lhs[2][3] * rhs[2]);
}

#endif
