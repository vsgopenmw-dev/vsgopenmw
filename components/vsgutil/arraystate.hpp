#ifndef VSGOPENMW_VSGUTIL_ARRAYSTATE_H
#define VSGOPENMW_VSGUTIL_ARRAYSTATE_H

#include <vsg/state/ArrayState.h>

namespace vsgUtil
{
    /*
     * Provides clone implementation using the Curiously Recurring Template Pattern.
     */
    template <class Derived>
    class ArrayState : public vsg::ArrayState
    {
        vsg::ref_ptr<vsg::ArrayState> clone(vsg::ref_ptr<vsg::ArrayState> arrayState) override
        {
            auto ret = vsg::ref_ptr{ new Derived(static_cast<const Derived&>(*this)) };
            static_cast<vsg::ArrayState&>(*ret) = *arrayState;
            return ret;
        }
    };
}

#endif
