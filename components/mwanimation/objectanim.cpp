#include "objectanim.hpp"

#include "play.hpp"

namespace MWAnim
{
    ObjectAnimation::ObjectAnimation(const Context& mwctx)
        : Object(mwctx)
    {
    }

    void ObjectAnimation::update(float dt)
    {
        autoPlay.update(dt);
        mUpdate.update(animation->time());
    }
}
