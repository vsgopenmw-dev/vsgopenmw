#ifndef VSGOPENMW_ANIMATION_MORPH_H
#define VSGOPENMW_ANIMATION_MORPH_H

#include "copydata.hpp"
#include "channel.hpp"

namespace Anim
{
    /*
     * Assigns morph weights.
     */
    class Morph : public CopyData<Morph, vsg::floatArray>
    {
    public:
        inline void apply(vsg::floatArray &array, float time) const
        {
            for (size_t i=0; i<weights.size(); ++i)
                array[i] = weights[i]->value(time);
        }
        std::vector<channel_ptr<float>> weights;
    };
}

#endif
