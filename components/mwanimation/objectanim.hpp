#ifndef VSGOPENMW_MWANIMATION_OBJECTANIM_H
#define VSGOPENMW_MWANIMATION_OBJECTANIM_H

#include "auto.hpp"

namespace MWAnim
{
    /*
     * Contains bone animation scene graph.
     */
    class ObjectAnimation : public Auto
    {
    public:
        ObjectAnimation(const Context& ctx);
        void update(float dt) override;

        inline void addController(const Anim::Controller* ctrl, vsg::Object* target)
        {
            if (ctrl->hints.autoPlay)
                autoPlay.add(ctrl, target);
            else
                mUpdate.add(ctrl, target);
        }

    protected:
        Anim::Update mUpdate;
    };
}

#endif
