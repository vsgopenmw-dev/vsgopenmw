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
        return {colorComponent(value, 0), colorComponent(value, 8), colorComponent(value, 16)};
    }

    inline vsg::vec4 rgbaColor(unsigned int value)
    {
        return {colorComponent(value, 0), colorComponent(value, 8), colorComponent(value, 16), colorComponent(value, 24)};
    }

    void brighten(vsg::vec4 &col, float minLuminance);
}

#endif
