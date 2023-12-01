#ifndef VSGOPENMW_ANIMATION_ANIMATION_H
#define VSGOPENMW_ANIMATION_ANIMATION_H

#include <functional>

#include "controllers.hpp"

namespace Anim
{
    class Context;

    /*
     * Links controllers.
     */
    struct Animation
    {
        Controllers controllers;
        std::vector<std::pair<Controller*, vsg::Object*>> clonedControllers; //{.updateOrder=controllers.updateOrder+1}

        void link(Context& ctx, std::function<void(const Controller*, vsg::Object*)> onLinked);
    };
}

#endif
