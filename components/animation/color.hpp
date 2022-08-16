#ifndef VSGOPENMW_ANIMATION_COLOR_H
#define VSGOPENMW_ANIMATION_COLOR_H

#include "copydata.hpp"
#include "channel.hpp"

namespace Anim
{
    /*
     * Writes color values.
     */
    class Color : public CopyData<Color>
    {
    public:
        size_t colorOffset = 0;
        channel_ptr<vsg::vec3> color;
        size_t alphaOffset = 0;
        channel_ptr<float> alpha;
        void apply(vsg::Data &data, float time) const
        {
            if (color)
            {
                vsg::vec3 &vec = *reinterpret_cast<vsg::vec3*>(reinterpret_cast<unsigned char*>(data.dataPointer())+colorOffset);
                vec = color->value(time);
            }
            if (alpha)
            {
                vsg::vec4 &a = *reinterpret_cast<vsg::vec4*>(reinterpret_cast<unsigned char*>(data.dataPointer())+alphaOffset);
                a.w = alpha->value(time);
            }
        };
    };
}

#endif
