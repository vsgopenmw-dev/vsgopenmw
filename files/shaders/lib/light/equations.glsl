#include "settings"

float lambert(vec3 light, vec3 normal)
{
    return max(dot(normal, light),0.0);
}

float lightRange(float intensity)
{
#if 0
return intensity*4;
#else
    float r = intensity * LightAttenuation_LinearRadiusMult;
    float attenuation = LightAttenuation_LinearValue/r;
    const float epsilon = 0.03;
    return 1/(epsilon*attenuation);
#endif
}

float attenuate(float intensity, float distance)
{
    float value = 0;
    if (LightAttenuation_UseConstant)
        value += LightAttenuation_ConstantValue;
    if (LightAttenuation_UseLinear)
    {
        float r = intensity * LightAttenuation_LinearRadiusMult;
        float attenuation = LightAttenuation_LinearValue;
        if (LightAttenuation_LinearMethod == 1)
            attenuation /= r;
        if (LightAttenuation_LinearMethod == 2)
            attenuation /= r * r;
        value += attenuation * distance;
    }
    if (LightAttenuation_UseQuadratic)
    {
        float r = intensity * LightAttenuation_QuadraticRadiusMult;
        float attenuation = LightAttenuation_QuadraticValue;
        if (LightAttenuation_QuadraticMethod == 1)
            attenuation /= r;
        if (LightAttenuation_QuadraticMethod == 2)
            attenuation /= r * r;
        value += attenuation * distance * distance;
    }
    return clamp(1.0 / value, 0.0, 1.0);
}
