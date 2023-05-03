struct PointLight
{
    vec4 colorIntensity;
    vec4 position;
};

layout(std430, set=VIEW_SET, binding=VIEW_LIGHTS_BINDING) readonly buffer Lights {
    PointLight pointLights[];
};
