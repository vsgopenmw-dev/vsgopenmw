#include "objectanim.hpp"

#include "play.hpp"

namespace MWAnim
{
    ObjectAnimation::ObjectAnimation(const Context& mwctx)
        : Auto(mwctx)
    {
    }

    void ObjectAnimation::update(float dt)
    {
        Auto::update(dt);
        mUpdate.update(animation->time());
    }
}
