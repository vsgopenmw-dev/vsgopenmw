#ifndef VSGOPENMW_ANIMATION_TRANSFORM_H
#define VSGOPENMW_ANIMATION_TRANSFORM_H

#include <vsg/nodes/Transform.h>

namespace Anim
{
    /*
     * Implements single-precision decomposed transform.
     */
    class Transform : public vsg::Inherit<vsg::Transform, Transform>
    {
    public:
        vsg::vec3 translation;
        vsg::mat3 rotation;
        vsg::vec3 scale{ 1.f, 1.f, 1.f };

        void setAttitude(const vsg::quat& q);

        template <class Matrix>
        void setRotation(const Matrix& m)
        {
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    rotation(i, j) = m(i, j);
        }

        void setScale(float s) { scale = { s, s, s }; }

        vsg::dmat4 transform(const vsg::dmat4& in) const final override { return t_transform(in); }

        template <typename T>
        inline vsg::t_mat4<T> t_transform(const vsg::t_mat4<T>& in) const
        {
            vsg::t_mat4<T> matrix = {
                rotation[0][0] * scale[0], rotation[0][1] * scale[0], rotation[0][2] * scale[0], 0,
                rotation[1][0] * scale[1], rotation[1][1] * scale[1], rotation[1][2] * scale[1], 0,
                rotation[2][0] * scale[2], rotation[2][1] * scale[2], rotation[2][2] * scale[2], 0,
                translation[0], translation[1], translation[2], 1
            };
            return in * matrix;
        }

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;
    };
}
namespace vsg
{
    VSG_type_name(Anim::Transform)
}

#endif
