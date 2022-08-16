#ifndef VSGOPENMW_MWANIMATION_DECORATE_H
#define VSGOPENMW_MWANIMATION_DECORATE_H

#include "clone.hpp"

namespace ESM
{
    class Light;
}
namespace MWAnim
{
    /*
     * Supplements graph.
     */
    PlaceholderResult decorate(vsg::ref_ptr<vsg::Node> &node, const ESM::Light *light, bool mirror=false);
}

#endif
