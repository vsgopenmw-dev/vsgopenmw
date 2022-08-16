out gl_PerVertex{ vec4 gl_Position; };

void main()
{
    vec2 vertex = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2 - 1;
    gl_Position = vec4(vertex, 0.0, 1.0);
}
