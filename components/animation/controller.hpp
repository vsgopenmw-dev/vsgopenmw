#ifndef VSGOPENMW_ANIMATION_CONTROLLER_H
#define VSGOPENMW_ANIMATION_CONTROLLER_H

#include <components/vsgutil/attachable.hpp>

namespace Anim
{
    struct Context;

    /*
     * Controller is an interface base class for animation logic. Implementations animate objects based on an input time value, e.g. applying the value returned by a Channel to some property of a target object.
     * Typically the action performed is purely a function of the input time as the Controller::run(..) method is const.
     * Controllers that have internal state that affects the outcome of actions should implement the link(..) and cloneIfRequired() methods.
     * Relevant subclasses are TController and TMutable that provide type safety for controlling objects of a particular type.
     */
    class Controller : public vsgUtil::Attachable<Controller>
    {
    public:
        using target_type = vsg::Object;
        static const std::string sAttachKey;

        virtual void run(vsg::Object& target, float time) const = 0;

        // Optionally links the controller to a specific scene graph. Unlinked controllers may be reused across scene graphs.
        virtual void link(Context&, vsg::Object&) {}

        virtual vsg::ref_ptr<Controller> cloneIfRequired() const { return {}; }

        // Hints can be set as a guide of what time values are appropriate to pass to the run(..) method.
        struct Hints
        {
            float duration = 0.f;
            bool autoPlay = false;
            int phaseGroup = 0; // = Group_NotRandom;
        } hints;
    };
}

#endif
