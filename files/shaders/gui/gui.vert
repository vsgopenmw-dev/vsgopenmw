struct Vertex
{
    float x;
    float y;
    float z;
    uint color;
    float u;
    float v;
};

#extension GL_EXT_scalar_block_layout : require
layout(scalar, set=1, binding=0) readonly buffer VertexBuffer{
    Vertex vertices[];
};

const int mask = int(0xff);
const ivec4 shift = ivec4(int(0), int(8), int(16), int(24));

vec4 unpackRGBA(uint data)
{
    return vec4( (float(((data >> shift.x) & mask)) / 255.0)
                ,(float(((data >> shift.y) & mask)) / 255.0)
                ,(float(((data >> shift.z) & mask)) / 255.0)
                ,(float(((data >> shift.w) & mask)) / 255.0));
}

layout(location = 1) out vec4 outColor;
layout(location = 2) out vec2 outTexCoord;
out gl_PerVertex{ vec4 gl_Position; };

void main()
{
    Vertex v = vertices[gl_VertexIndex];
    gl_Position = vec4(v.x, -v.y, 0.0, 1.0);
    outColor = unpackRGBA(v.color);
    outTexCoord = vec2(v.u, v.v);;
}
