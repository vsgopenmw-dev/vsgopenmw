#ifndef VSGOPENMW_ANIMATION_TRANSFORMUPDATE_H
#define VSGOPENMW_ANIMATION_TRANSFORMUPDATE_H

#include "transformcontroller.hpp"

namespace Anim
{
    using TransformControllers = std::vector<std::pair<const TransformController*, Transform*>>;

    /*
     * Uses external data.
     */
    struct ExternalTransformUpdate
    {
        ExternalTransformUpdate(const TransformControllers *l, const float *t) : list(l), time(t) {}
        ExternalTransformUpdate() : list(&sEmpty), time(&sNullTime) {}
        inline void update()
        {
            for (auto &[ctrl, target] : *list)
                ctrl->run(*target, *time);
        }
        static const float sNullTime;
        static const TransformControllers sEmpty;
        const TransformControllers *list;
        const float *time;
    };
}

#endif
