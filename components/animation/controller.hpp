#ifndef VSGOPENMW_ANIMATION_CONTROLLER_H
#define VSGOPENMW_ANIMATION_CONTROLLER_H

#include <components/vsgutil/attachable.hpp>

namespace Anim
{
    struct Context;

    /*
     * Controls objects as a function of time.
     */
    class Controller : public vsgUtil::Attachable<Controller>
    {
    public:
        using target_type = vsg::Object;
        static const std::string sAttachKey;

        virtual void run(vsg::Object &target, float time) const = 0;

        // Optionally links the controller to a specific scene graph. Unlinked controllers may be reused across scene graphs.
        virtual void link(Context &, vsg::Object &) {}

        virtual vsg::ref_ptr<Controller> cloneIfRequired() const { return {}; }

        // Optionally describes desired usage.
        struct Hints
        {
            float duration = 0.f;
            bool autoPlay = false;
        } hints;
    };
}

#endif
