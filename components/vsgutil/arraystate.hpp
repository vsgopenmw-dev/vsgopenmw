#ifndef VSGOPENMW_VSGUTIL_ARRAYSTATE_H
#define VSGOPENMW_VSGUTIL_ARRAYSTATE_H

#include <vsg/state/ArrayState.h>

namespace vsgUtil
{
    /*
     * Adds convenience functionality to vsg::ArrayState, such as an automatically provided clone(..) implementation, based on the Curiously Recurring Template Pattern.
     */
    template <class Derived>
    class ArrayState : public vsg::ArrayState
    {
    public:
        vsg::ref_ptr<vsg::ArrayState> clone(vsg::ref_ptr<vsg::ArrayState> arrayState)
        {
            auto ret = vsg::ref_ptr{ new Derived(static_cast<const Derived&>(*this)) };
            static_cast<vsg::ArrayState&>(*ret) = *arrayState;
            return ret;
        }

        template<typename... Args>
        static vsg::ref_ptr<Derived> create(Args&&... args)
        {
            return vsg::ref_ptr<Derived>(new Derived(args...));
        }

        /*
         * Provides proxy_vertices persisting across clones.
         */
        vsg::vec3Array* getOrCreateProxyVertices(vsg::Object* parent, size_t minSize)
        {
            auto proxy = parent->getObject<vsg::vec3Array>("proxy");
            if (!proxy || proxy->size() < minSize)
            {
                auto array = vsg::vec3Array::create(minSize);
                parent->setObject("proxy", array);
                proxy = array;
            }
            return proxy;
        }
    };
}

#endif
