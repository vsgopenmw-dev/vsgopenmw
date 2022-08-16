#ifndef VSGOPENMW_PIPELINE_MATERIAL_H
#define VSGOPENMW_PIPELINE_MATERIAL_H

#include <vsg/maths/vec4.h>

#include "descriptorvalue.hpp"
#include "bindings.hpp"

namespace Pipeline
{
    namespace Data
    {
        using vsg::vec4;
        #include <files/shaders/lib/material/data.glsl>
    }

    /*
     * Provides convenience functions.
     */
    class Material : public DynamicDescriptorValue<Data::Material>
    {
    public:
        Material() : DynamicDescriptorValue(Descriptors::MATERIAL_BINDING) { value() = createDefault(); }
        void setAlpha(float a)
        {
            value().diffuse.a = a;
        }
        static Data::Material createDefault()
        {
            return {vsg::vec4(1,1,1,1), vsg::vec4(1,1,1,1), vsg::vec4(0,0,0,1), vsg::vec4(0,0,0,0), 0.f, 1.f, 0.f, 0.f};
        }
        static vsg::ref_ptr<value_type> createDefaultValue()
        {
            return value_type::create(createDefault());
        }

        static bool isDefault(const Data::Material &mat)
        {
            static const auto defaultMat = createDefault();
            return std::memcmp(&mat, &defaultMat, sizeof(Data::Material)) == 0;
        }
    };
}

#endif
