#ifndef VSGOPENMW_VSGUTIL_CLONEABLE_H
#define VSGOPENMW_VSGUTIL_CLONEABLE_H

#include <vsg/core/Value.h>

namespace vsgUtil
{
    /*
     * Clones itself.
     */
    class Cloneable
    {
    public:
        virtual ~Cloneable() {}
        virtual vsg::ref_ptr<vsg::Object> clone() const = 0;
    };

    /*
     * Adds clone functionality to vsg::Value.
     */
    template <class T>
    class Value : public Cloneable, public vsg::Value<T>
    {
    public:
        Value() {}
        Value(const typename vsg::Value<T>::value_type &v)
            : vsg::Value<T>(v) {}

        template <typename... Args>
        static vsg::ref_ptr<vsgUtil::Value<T>> create(Args&&... args)
        {
            return vsg::ref_ptr{new vsgUtil::Value<T>(args...)};
        }

        vsg::ref_ptr<vsg::Object> clone() const override
        {
            return vsg::ref_ptr{new vsg::Value<T>(*this)};
        }
    };
}

#endif
