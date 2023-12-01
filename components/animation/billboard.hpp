#ifndef VSGOPENMW_ANIMATION_BILLBOARD_H
#define VSGOPENMW_ANIMATION_BILLBOARD_H

#include "transform.hpp"

#include <components/vsgutil/transform.hpp>

namespace Anim
{
    /*
     * Orients towards camera more flexibly than shader billboards.
     */
    class Billboard : public vsg::Inherit<vsg::Transform, Billboard>
    {
    public:
        // enum BillboardType;
        vsg::dmat4 transform(const vsg::dmat4& in_matrix) const override
        {
            auto matrix = in_matrix;
            // preserve scale
            auto xAxis = vsgUtil::transform3x3(vsg::dvec3(1,0,0), in_matrix);
            auto yAxis = vsgUtil::transform3x3(vsg::dvec3(0,1,0), in_matrix);
            vsgUtil::setRotation(matrix, vsg::dmat3(
                vsg::dvec3(vsg::length(xAxis), 0.0, 0.0),
                vsg::dvec3(0.0, vsg::length(yAxis), 0.0),
                vsg::dvec3(0.0, 0.0, 1.0)
            ));
            return matrix;
        }
    };
}

#endif
