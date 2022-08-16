#ifndef VSGOPENMW_ANIMATION_TMUTABLE_H
#define VSGOPENMW_ANIMATION_TMUTABLE_H

#include "controller.hpp"

namespace Anim
{
    /*
     * Statefully controls an object.
     */
    template <class Derived, class Target>
    class TMutable : public Controller
    {
    public:
        using target_type = Target;
        void run(vsg::Object &target, float time) const final override
        {
            const_cast<Derived&>(static_cast<const Derived&>(*this)).apply(static_cast<Target&>(target), time);
        }
        vsg::ref_ptr<Controller> cloneIfRequired() const override { return vsg::ref_ptr{new Derived(static_cast<const Derived&>(*this))}; }
        void attachTo(vsg::Object& to) = delete;
        inline void attachTo(Target& to)
        {
            Controller::attachTo(to);
        }
    };
}

#endif
