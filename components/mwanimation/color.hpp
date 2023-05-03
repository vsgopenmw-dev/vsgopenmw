#ifndef VSGOPENMW_MWANIMATION_COLOR_H
#define VSGOPENMW_MWANIMATION_COLOR_H

#include <vsg/maths/vec4.h>

namespace MWAnim
{
    inline float colorComponent(unsigned int value, unsigned int shift)
    {
        return float((value >> shift) & 0xFFu) / 255.0f;
    }

    inline vsg::vec3 rgbColor(unsigned int value)
    {
        return { colorComponent(value, 0), colorComponent(value, 8), colorComponent(value, 16) };
    }

    inline vsg::vec4 rgbaColor(unsigned int value)
    {
        return { colorComponent(value, 0), colorComponent(value, 8), colorComponent(value, 16),
            colorComponent(value, 24) };
    }

    inline vsg::vec4 color(uint8_t r, uint8_t g, uint8_t b)
    {
        return { r / 255.f, g / 255.f, b / 255.f, 1 };
    }

    void brighten(vsg::vec4& col, float minLuminance);
}

#endif
