#ifndef VSGOPENMW_ANIMATION_UPDATE_H
#define VSGOPENMW_ANIMATION_UPDATE_H

#include <vector>

#include "controller.hpp"
#include "controllers.hpp"

namespace Anim
{
    /*
     * Runs controllers.
     */
    struct Update
    {
        Controllers controllers;
        inline void clear()
        {
            controllers.clear();
        }
        inline void add(const Controller* ctrl, vsg::Object* o)
        {
            controllers.emplace_back(ctrl, o);
        }
        inline void update(float time)
        {
            for (auto& [ctrl, target] : controllers)
                ctrl->run(*target, time);
        }
    };

    /*
     * Implements an incrementing timer.
     */
    struct AutoUpdate : public Update
    {
        float timer = 0.f;
        inline void update(float dt)
        {
            timer += dt;
            Update::update(timer);
        }
    };
}

#endif
