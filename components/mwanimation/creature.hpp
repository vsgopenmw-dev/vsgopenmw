#ifndef VSGOPENMW_MWANIMATION_CREATURE_H
#define VSGOPENMW_MWANIMATION_CREATURE_H

#include "actor.hpp"

namespace MWAnim
{
    class Context;

    /*
     * Loads simple creature animations.
     */
    class Creature : public Actor
    {
        Anim::Update mUpdate;
    public:
        Creature(const Context &ctx, bool biped, const std::string &model);
        void update(float dt) override;
    };
}

#endif
