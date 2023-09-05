#ifndef VSGOPENMW_ANIMATION_DELTATIME_H
#define VSGOPENMW_ANIMATION_DELTATIME_H

namespace Anim
{
    struct DeltaTime
    {
        float last = 0;
        float get(float current)
        {
            float delta = current - last;
            if (delta < 0)
                delta = 0;
            last = current;
            return delta;
        }
    };
}

#endif
