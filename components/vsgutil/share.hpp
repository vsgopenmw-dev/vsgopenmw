#ifndef VSGOPENMW_VSGUTIL_SHARE_H
#define VSGOPENMW_VSGUTIL_SHARE_H

#include <vsg/core/ref_ptr.h>

namespace vsgUtil
{
    template <class T, class F, typename ... Args>
    vsg::ref_ptr<T> share(F f, Args&& ... args)
    {
        static const auto ret = f(args...);
        return ret;
    }

    template <class T>
    vsg::ref_ptr<T> shareDefault()
    {
        static const auto ret = T::create();
        return ret;
    }
}

#endif
