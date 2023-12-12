#ifndef VSGOPENMW_MWRENDER_ITEMLIGHT_H
#define VSGOPENMW_MWRENDER_ITEMLIGHT_H

#include "../mwworld/ptr.hpp"

namespace ESM
{
    class Light;
}
namespace MWAnim
{
    class Object;
}
namespace MWRender
{
    void addItemLightsAndListener(MWAnim::Object& obj, MWWorld::ContainerStore& store);
    void removeListener(MWWorld::ContainerStore& store);
    void addItemLightIfRequired(MWAnim::Object& obj, const MWWorld::ConstPtr& item);
    void addItemLight(MWAnim::Object& obj, const MWWorld::ConstPtr& item, const ESM::Light& esmLight);
    void removeItemLight(MWAnim::Object& obj, const MWWorld::ConstPtr& item);
}

#endif
