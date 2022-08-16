#ifndef VSGOPENMW_ANIMATION_STATESWITCH_H
#define VSGOPENMW_ANIMATION_STATESWITCH_H

#include <vsg/state/StateSwitch.h>
#include <vsg/nodes/Switch.h>

#include "tcontroller.hpp"
#include "channel.hpp"

namespace Anim
{
    class StateSwitch : public TController<StateSwitch, vsg::StateSwitch>
    {
    public:
        void apply(vsg::StateSwitch &group, float time) const
        {
            size_t active = index->value(time);
            for (size_t i=0; i<group.children.size(); ++i)
                group.children[i].mask = vsg::boolToMask(i==active);
        }
        channel_ptr<size_t> index;
    };
}

#endif
