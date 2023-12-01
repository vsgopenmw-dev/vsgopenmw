#ifndef VSGOPENMW_ANIMATION_ANIMATION_H
#define VSGOPENMW_ANIMATION_ANIMATION_H

#include <functional>

#include "controllers.hpp"

namespace Anim
{
    class Context;

    /*
     * Animation is a container class for a list of controllers and the objects they target.
     * Stateful controllers are kept in a separate container that is not const so they can be set up before use.
     */
    struct Animation
    {
        Controllers controllers;
        std::vector<std::pair<Controller*, vsg::Object*>> clonedControllers; //{.updateOrder=controllers.updateOrder+1}

        // Run the Controller::link(..) procedure for all controller/object pairs and pass them to the specified callback.
        void link(Context& ctx, std::function<void(const Controller*, vsg::Object*)> onLinked);
    };
}

#endif
