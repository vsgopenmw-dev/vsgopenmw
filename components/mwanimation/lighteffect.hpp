#ifndef VSGOPENMW_MWANIMATION_LIGHTEFFECT_H
#define VSGOPENMW_MWANIMATION_LIGHTEFFECT_H

#include <vsg/nodes/Group.h>

namespace MWAnim
{
    /*
     * Optionally lights node.
     */
    void setLightEffect(vsg::Group &group, float intensity);
}

#endif
