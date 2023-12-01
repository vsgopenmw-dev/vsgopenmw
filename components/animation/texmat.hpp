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
        channel_ptr<float> translate[2];
        channel_ptr<float> scale[2];

        void apply(vsg::mat4Value& value, float time) const
        {
            vsg::vec2 s(scale[0]->value(time), scale[1]->value(time));
            vsg::vec2 trans(translate[0]->value(time), translate[1]->value(time));
            auto& mat = value.value();
            mat(0,0) = s[0];
            mat(1,1) = s[1];
            mat(3,0) = s[0] * -0.5f + 0.5f + trans.x;
            mat(3,1) = s[1] * -0.5f + 0.5f + trans.y;
        }
    };
}
#endif
