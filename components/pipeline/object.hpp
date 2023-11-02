#ifndef VSGOPENMW_PIPELINE_OBJECT_H
#define VSGOPENMW_PIPELINE_OBJECT_H

#include "descriptorvalue.hpp"
#include "objectdata.hpp"

namespace Pipeline
{
    struct Object : public DynamicDescriptorValue<Data::Object>
    {
        Object()
            : DynamicDescriptorValue<Data::Object>(0)
        {
            value().alpha = 1;
        }
    };
}

#endif
