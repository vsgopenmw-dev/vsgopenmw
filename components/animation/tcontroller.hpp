#ifndef VSGOPENMW_ANIMATION_TCONTROLLER_H
#define VSGOPENMW_ANIMATION_TCONTROLLER_H

#include "controller.hpp"

namespace Anim
{
    /*
     * Specifies controlled objects type.
     */
    template <class Derived, class Target>
    class TController : public Controller
    {
    public:
        using target_type = Target;
        void run(vsg::Object &target, float time) const final override
        {
            static_cast<const Derived&>(*this).apply(static_cast<Target&>(target), time);
        }
        void attachTo(vsg::Object &to) = delete;
        inline void attachTo(Target &to)
        {
            Controller::attachTo(to);
        }

        void link(Context &, vsg::Object &) /*=delete*/ final override {}
        vsg::ref_ptr<Controller> cloneIfRequired() const final override { return {}; }
    };
}

#endif
