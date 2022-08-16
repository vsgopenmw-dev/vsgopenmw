#ifndef VSGOPENMW_ANIMATION_SWITCH_H
#define VSGOPENMW_ANIMATION_SWITCH_H

#include <vsg/nodes/Switch.h>

#include "tcontroller.hpp"
#include "channel.hpp"

namespace Anim
{
    class Switch : public TController<Switch, vsg::Switch>
    {
    public:
        Switch(channel_ptr<bool> t) : toggle(t) {}
        void apply(vsg::Switch &group, float time) const
        {
            group.setAllChildren(toggle->value(time));
        }
        channel_ptr<bool> toggle;
    };
}

#endif
