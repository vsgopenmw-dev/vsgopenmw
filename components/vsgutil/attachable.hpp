#ifndef VSGOPENMW_VSGUTIL_ATTACHABLE_H
#define VSGOPENMW_VSGUTIL_ATTACHABLE_H

#include <vsg/core/Object.h>

namespace vsgUtil
{
    /*
     * Attaches to other objects.
     */
    template <class Derived, class Target=vsg::Object>
    class Attachable : public vsg::Object
    {
    public:
        inline static Derived *get(Target &attachedTo)
        {
            return static_cast<Derived*>(attachedTo.getObject(Derived::sAttachKey));
        }
        inline static Derived *getOrCreate(Target &attachedTo)
        {
            if (auto obj = get(attachedTo))
                return obj;
            auto obj = vsg::ref_ptr{new Derived};
            obj->attachTo(attachedTo);
            return obj;
        }
        inline static const Derived *get(const Target &attachedTo)
        {
            return static_cast<const Derived*>(attachedTo.getObject(Derived::sAttachKey));
        }
        inline void attachTo(Target &to)
        {
            to.setObject(Derived::sAttachKey, this);
        }
    };
}

#endif
