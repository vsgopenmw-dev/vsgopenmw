#ifndef VSGOPENMW_ANIMATION_MASK_H
#define VSGOPENMW_ANIMATION_MASK_H

#include <vsg/core/Mask.h>

namespace Anim
{
    /// Mask object specifies the default masks to apply to optional nodes, and can be used to turn off rendering of particle effects when not desired.
    struct Mask
    {
        vsg::Mask particle{ vsg::MASK_OFF };
    };
}

#endif
