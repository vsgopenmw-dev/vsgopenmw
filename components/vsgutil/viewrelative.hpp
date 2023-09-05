#ifndef VSGOPENMW_VSGUTIL_VIEWRELATIVE_H
#define VSGOPENMW_VSGUTIL_VIEWRELATIVE_H

#include <vsg/nodes/Transform.h>

namespace vsgUtil
{
    /*
     * Makes subgraph relative to the camera position.
     */
    struct ViewRelative : public vsg::Transform
    {
        vsg::dvec3 resetAxes{ 0, 0, 0 };
        vsg::dmat4 transform(const vsg::dmat4& mat) const override
        {
            auto m = mat;
            m[3] *= vsg::dvec4(resetAxes, 1);
            return m;
        }
    };
}

#endif
