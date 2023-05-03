#ifndef VSGOPENMW_ANIMATION_MORPH_H
#define VSGOPENMW_ANIMATION_MORPH_H

#include <vsg/core/Array.h>

#include "channel.hpp"
#include "updatedata.hpp"

namespace Anim
{
    /*
     * Assigns morph weights.
     */
    class Morph : public UpdateData<Morph, vsg::floatArray>
    {
    public:
        inline void apply(vsg::floatArray& array, float time) const
        {
            for (size_t i = 0; i < weights.size(); ++i)
                array[i] = weights[i]->value(time);
        }
        std::vector<channel_ptr<float>> weights;
    };
}

#endif
