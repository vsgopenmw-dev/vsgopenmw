layout(location=0) in vec3 inVertex;
layout(location=1) in vec4 inColor;
layout(location=2) in vec2 inTexCoord;

layout(location=1) out vec4 outColor;
layout(location=2) out vec2 outTexCoord;
out gl_PerVertex{ vec4 gl_Position; };

void main()
{
    gl_Position = vec4(inVertex.x, -inVertex.y, 0.0, 1.0);
    outColor = inColor;
    outTexCoord = inTexCoord;
}
