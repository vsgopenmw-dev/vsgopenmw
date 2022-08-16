#ifndef VSGOPENMW_ANIMATION_TRANSFORMCONTROLLER_H
#define VSGOPENMW_ANIMATION_TRANSFORMCONTROLLER_H

#include "tcontroller.hpp"
#include "channel.hpp"
#include "transform.hpp"

namespace Anim
{
    class TransformController : public TController<TransformController, Transform>
    {
    public:
        channel_ptr<vsg::quat> rotate;
        channel_ptr<vsg::vec3> translate;
        channel_ptr<float> scale;
        void apply(Transform &transform, float time) const;
    };
}

#endif
