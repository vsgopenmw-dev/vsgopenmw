#ifndef VSGOPENMW_MWANIMATION_POSITION_H
#define VSGOPENMW_MWANIMATION_POSITION_H

#include <vsg/maths/quat.h>

#include <components/esm/defs.hpp>

namespace MWAnim
{
    inline vsg::quat quat(const ESM::Position &p)
    {
        return vsg::quat(p.rot[2], vsg::vec3(0, 0, -1))
            * vsg::quat(p.rot[1], vsg::vec3(0, -1, 0))
            * vsg::quat(p.rot[0], vsg::vec3(-1, 0, 0));
    }
    inline vsg::quat reverseQuat(const ESM::Position &p)
    {
        return vsg::quat(p.rot[0], vsg::vec3(-1, 0, 0))
            * vsg::quat(p.rot[1], vsg::vec3(0, -1, 0))
            * vsg::quat(p.rot[2], vsg::vec3(0, 0, 1));
    }
    inline vsg::quat zQuat(const ESM::Position &p)
    {
        return vsg::quat(p.rot[2], vsg::vec3(0, 0, -1));
    }

    inline vsg::vec3 forward(const ESM::Position &p)
    {
        return zQuat(p) * vsg::vec3(0,1,0);
    }
}

#endif
