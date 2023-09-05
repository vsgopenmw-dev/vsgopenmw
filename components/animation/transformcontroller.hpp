#ifndef VSGOPENMW_ANIMATION_TRANSFORMCONTROLLER_H
#define VSGOPENMW_ANIMATION_TRANSFORMCONTROLLER_H

#include "channel.hpp"
#include "tcontroller.hpp"
#include "transform.hpp"

namespace Anim
{
    class TransformController : public TController<TransformController, Transform>
    {
    public:
        channel_ptr<vsg::quat> rotate;
        channel_ptr<vsg::vec3> translate;
        channel_ptr<float> scale;
        void apply(Transform& transform, float time) const
        {
            if (rotate)
                transform.setAttitude(rotate->value(time));
            if (translate)
                transform.translation = translate->value(time);
            if (scale)
                transform.setScale(scale->value(time));
        }
    };
}

#endif
