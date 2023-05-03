#ifndef VSGOPENMW_ANIMATION_UPDATEDATA_H
#define VSGOPENMW_ANIMATION_UPDATEDATA_H

#include "tcontroller.hpp"
#include "tmutable.hpp"

#include <vsg/core/Data.h>

namespace Anim
{
    /*
     * Dirties data updated by const Derived.
     */
    template <class Derived, class DataType = vsg::Data>
    class UpdateData : public TController<UpdateData<Derived, DataType>, DataType>
    {
    public:
        void apply(DataType& data, float time) const
        {
            /*if (*/ static_cast<const Derived&>(*this).apply(data, time);
            data.dirty();
        }
        void attachTo(DataType& to)
        {
            to.properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;
            Controller::attachTo(to);
        }
    };

    /*
     * Dirties data statefully updated by Derived.
     */
    template <class Derived, class DataType = vsg::Data>
    class MUpdateData : public TMutable<MUpdateData<Derived, DataType>, DataType>
    {
    public:
        void apply(DataType& data, float time)
        {
            /*if (*/ static_cast<Derived&>(*this).apply(data, time);
            data.dirty();
        }
        void attachTo(DataType& to)
        {
            to.properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;
            Controller::attachTo(to);
        }
        vsg::ref_ptr<Controller> cloneIfRequired() const override
        {
            return vsg::ref_ptr{ new Derived(static_cast<const Derived&>(*this)) };
        }
    };
}

#endif
