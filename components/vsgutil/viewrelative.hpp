#ifndef VSGOPENMW_VSGUTIL_VIEWRELATIVE_H
#define VSGOPENMW_VSGUTIL_VIEWRELATIVE_H

#include <vsg/nodes/Transform.h>

namespace vsgUtil
{
    class ViewRelative : public vsg::Transform
    {
        vsg::dmat4 transform(const vsg::dmat4 &mat) const override
        {
            auto m = mat;
            m[3] = {0,0,0,1};
            return m;
        }
    };
}

#endif
