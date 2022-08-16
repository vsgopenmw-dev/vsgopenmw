#ifndef VSGOPENMW_VSGUTIL_SEARCHBYTYPE_H
#define VSGOPENMW_VSGUTIL_SEARCHBYTYPE_H

#include <stdexcept>

#include "traverse.hpp"

namespace vsgUtil
{
    template <class T, class Base=vsg::Visitor, bool unique=true, bool optional=false>
    class Search : public Base
    {
    public:
        vsg::ref_ptr<T> found;

        void apply(vsg::Object &o) override
        {
            if (unique || !found)
                Base::apply(o);
        }
        void apply(T &t) override
        {
            if (unique && found)
                throw std::runtime_error(std::string("Search<") + vsg::type_name<T>() + ">: unique && found");
            found = &t;
        }
        vsg::ref_ptr<T> search(vsg::Object &o)
        {
            o.accept(*this);
            if (!optional && !found)
                throw std::runtime_error(std::string("Search<") + vsg::type_name<T>() + ">: !optional && !found");
            return std::move(found);
        }
    };

    /*
     * Returns first object of specified type or nullptr.
     */
    template<class T, class Base=Traverse>
    vsg::ref_ptr<T> searchFirst(vsg::Object &o) { return Search<T, Base, false, true>().search(o); }

    /*
     * Returns the object of specified type or nullptr. Throws if there are multiple such objects.
     */
    template<class T, class Base=Traverse>
    vsg::ref_ptr<T> search(vsg::Object &o) { return Search<T, Base, true, true>().search(o); }

    /*
     * Returns first object of specified type. Throws if no such object is found.
     */
    template<class T, class Base=Traverse>
    vsg::ref_ptr<T> findFirst(vsg::Object &o) { return Search<T, Base, false, false>().search(o); }

    /*
     * Returns the object of specified type. Throws if no such object is found or there are multiple such objects.
     */
    template<class T, class Base=Traverse>
    vsg::ref_ptr<T> find(vsg::Object &o) { return Search<T, Base, true, false>().search(o); }
}

#endif
