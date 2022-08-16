layout(binding = 0) uniform sampler2D diffuseMap;
layout(location = 1) in vec4 vertColor;
layout(location = 2) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(diffuseMap, texCoord) * vertColor;
}

