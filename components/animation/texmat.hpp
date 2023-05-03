#ifndef VSGOPENMW_ANIMATION_TEXMAT_H
#define VSGOPENMW_ANIMATION_TEXMAT_H

#include <vsg/core/Value.h>

#include "channel.hpp"
#include "updatedata.hpp"

namespace Anim
{
    /*
     * Assembles texture coordinate matrix.
     */
    class TexMat : public UpdateData<TexMat, vsg::mat4Value>
    {
    public:
        void apply(vsg::mat4Value& value, float time) const;
        channel_ptr<float> translate[2];
        channel_ptr<float> scale[2];
    };
}
#endif
