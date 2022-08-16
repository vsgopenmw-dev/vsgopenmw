#ifndef VSGOPENMW_VIEW_DEFAULTSTATE_H
#define VSGOPENMW_VIEW_DEFAULTSTATE_H

#include <vsg/nodes/StateGroup.h>

namespace View
{
    vsg::ref_ptr<vsg::StateGroup> createDefaultState(vsg::ref_ptr<vsg::DescriptorSet> viewDescriptorSet);
}

#endif
