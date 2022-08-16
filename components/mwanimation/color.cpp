#include "color.hpp"

namespace MWAnim
{
    void brighten(vsg::vec4 &col, float minLuminance)
    {
        constexpr float pR = 0.2126;
        constexpr float pG = 0.7152;
        constexpr float pB = 0.0722;

        // we already work in linear RGB so no conversions are needed for the luminosity function
        float relativeLuminance = pR*col.r + pG*col.g + pB*col.b;
        if (relativeLuminance < minLuminance)
        {
            // brighten col so it reaches the minimum threshold but no more, we want to mess with content data as least we can
            float targetBrightnessIncreaseFactor = minLuminance / relativeLuminance;
            if (col.r == 0.f && col.g == 0.f && col.b == 0.f)
                col = {minLuminance, minLuminance, minLuminance, col.a};
            else
                col *= targetBrightnessIncreaseFactor;
        }
    }
}
