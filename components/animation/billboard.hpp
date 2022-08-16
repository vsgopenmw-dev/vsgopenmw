#ifndef VSGOPENMW_ANIMATION_BILLBOARD_H
#define VSGOPENMW_ANIMATION_BILLBOARD_H

#include "transform.hpp"

namespace Anim
{
    /*
     * Orients towards camera more flexibly than shader billboards.
     */
    class Billboard : public vsg::Transform
    {
    public:
        //enum BillboardType;
        vsg::dmat4 transform(const vsg::dmat4 &in) const override;
    };
}

#endif
