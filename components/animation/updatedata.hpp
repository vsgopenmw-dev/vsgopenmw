#ifndef VSGOPENMW_ANIMATION_UPDATEDATA_H
#define VSGOPENMW_ANIMATION_UPDATEDATA_H

#include "controller.hpp"

#include <vsg/core/Data.h>

namespace Anim
{
    /*
     * Dirties data updated by const Derived.
     */
    template <class Derived, class DataType = vsg::Data>
    class UpdateData : public Controller
    {
    public:
        using target_type = DataType;
        void run(vsg::Object& target, float time) const final override
        {
            auto& data = static_cast<DataType&>(target);
            /*if (*/ static_cast<const Derived&>(*this).apply(data, time);
            data.dirty();
        }
        vsg::ref_ptr<Controller> cloneIfRequired() const final override { return {}; }
        void link(Context&, vsg::Object&) /*=delete*/ final override {}
        void attachTo(vsg::Object& to) = delete;
        void attachTo(DataType& to)
        {
            to.properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;
            Controller::attachTo(to);
        }
        template<typename... Args>
        static vsg::ref_ptr<Derived> create(Args&&... args)
        {
            return vsg::ref_ptr<Derived>(new Derived(args...));
        }
    };

    /*
     * Dirties data statefully updated by Derived.
     */
    template <class Derived, class DataType = vsg::Data>
    class MUpdateData : public Controller
    {
    public:
        using target_type = DataType;
        void run(vsg::Object& target, float time) const final override
        {
            auto& data = static_cast<DataType&>(target);
            /*if (*/ const_cast<Derived&>(static_cast<const Derived&>(*this)).apply(data, time);
            data.dirty();
        }
        void attachTo(vsg::Object& to) = delete;
        void attachTo(DataType& to)
        {
            to.properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;
            Controller::attachTo(to);
        }
        vsg::ref_ptr<Controller> cloneIfRequired() const override
        {
            return vsg::ref_ptr{ new Derived(static_cast<const Derived&>(*this)) };
        }
        template<typename... Args>
        static vsg::ref_ptr<Derived> create(Args&&... args)
        {
            return vsg::ref_ptr<Derived>(new Derived(args...));
        }
    };
}

#endif
