#ifndef VSGOPENMW_MWANIMATION_OBJECTANIM_H
#define VSGOPENMW_MWANIMATION_OBJECTANIM_H

#include "auto.hpp"

namespace MWAnim
{
    class Context;

    /*
     * Creates simple animation scene graph.
     */
    class ObjectAnimation : public Auto {
    public:
        ObjectAnimation(const Context &ctx, const std::string &model, const ESM::Light *light);
        void update(float dt) override;
    private:
        Anim::Update mUpdate;
   };
}

#endif
