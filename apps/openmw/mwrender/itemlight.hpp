#ifndef VSGOPENMW_MWRENDER_ITEMLIGHT_H
#define VSGOPENMW_MWRENDER_ITEMLIGHT_H

#include "../mwworld/ptr.hpp"

#include <vsg/nodes/Group.h>

#include <components/animation/controllers.hpp>

namespace ESM
{
    class Light;
}
namespace MWRender
{
    void addItemLightsAndListener(vsg::Group &node, MWWorld::ContainerStore &store);
    void removeListener(MWWorld::ContainerStore &store);
    void addItemLightIfRequired(vsg::Group &node, const MWWorld::ConstPtr &item);
    void addItemLight(vsg::Group &node, const MWWorld::ConstPtr &item, const ESM::Light &esmLight);
    void removeItemLight(vsg::Group &node, const MWWorld::ConstPtr &item);
    void updateItemLights(vsg::Group &node, float dt);
}

#endif
