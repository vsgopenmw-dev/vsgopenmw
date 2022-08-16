#include "transformcontroller.hpp"

namespace Anim
{
    void TransformController::apply(Transform &transform, float time) const
    {
        if (rotate)
            transform.setAttitude(rotate->value(time));
        if (scale)
            transform.setScale(scale->value(time));
        if (translate)
            transform.translation = translate->value(time);
    }
}
