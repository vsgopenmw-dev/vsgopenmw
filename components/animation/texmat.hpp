#ifndef VSGOPENMW_ANIMATION_TEXMAT_H
#define VSGOPENMW_ANIMATION_TEXMAT_H

#include "channel.hpp"
#include "copydata.hpp"

namespace Anim
{
    /*
     * Assembles texture coordinate matrix.
     */
    class TexMat : public CopyData<TexMat, vsg::mat4Value>
    {
    public:
        void apply(vsg::mat4Value &value, float time) const;
        channel_ptr<float> translate[2];
        channel_ptr<float> scale[2];
    };
}
#endif

