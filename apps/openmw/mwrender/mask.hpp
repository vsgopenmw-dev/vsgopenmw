#ifndef VSGOPENMW_MWRENDER_MASK_H
#define VSGOPENMW_MWRENDER_MASK_H

#include <vsg/core/Mask.h>

namespace MWRender
{
    //vsgopenmw-mask-policy=PREFER_DETACH_NODE
    enum Mask : vsg::Mask
    {
        // object masks
        Mask_Actor = (1<<3),
        Mask_Player = (1<<4),
        Mask_FirstPerson = (1<<9),
        Mask_Object = (1<<10),
        Mask_Effect = (1<<11),

        // view masks
        Mask_GUI = (1<<15),

        // leaf masks
        Mask_Particle = (1<<16),
    };
}

#endif
