#include "texmat.hpp"

namespace Anim
{
    void TexMat::apply(vsg::mat4Value &value, float time) const
    {
        vsg::vec2 origin(0.5f, 0.5f);
        vsg::vec2 s(scale[0]->value(time), scale[1]->value(time));
        vsg::vec2 trans(translate[0]->value(time), translate[1]->value(time));
        vsg::mat4 &mat = value.value();
        mat = vsg::mat4();
        mat[3] = vsg::vec4(origin, 0.f, 1.f);
        for (int c=0;c<2;++c)
            for (int r=0; r<2;++r)
                mat(c,r) *= s[c];
        for (int c = 0; c < 2; ++c)
            for (int r=0;r<2;++r)
                mat(3,r) += -origin[c]*mat(c,r);
        mat[3] += vsg::vec4(trans, 0.f, 1.f);
    }
}
