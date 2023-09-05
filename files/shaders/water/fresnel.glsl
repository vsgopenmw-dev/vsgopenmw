float fresnel_dielectric(vec3 incoming, vec3 normal, float eta)
{
    float c = abs(dot(incoming, normal));
    float g = eta * eta - 1.0 + c * c;
    if(g > 0.0)
    {
        g = sqrt(g);
        float a =(g - c)/(g + c);
        float b =(c *(g + c)- 1.0)/(c *(g - c)+ 1.0);
        return 0.5 * a * a * (1.0 + b * b);
    }
    else
        return 1.0; // TIR (no refracted component)
}
