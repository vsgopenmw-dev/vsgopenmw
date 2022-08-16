#include "billboard.hpp"

namespace Anim
{
    vsg::dmat4 Billboard::transform(const vsg::dmat4 &in_modelView) const
    {
        auto modelView = in_modelView;
        // attempt to preserve scale
        float mag[3];
        for (int i=0; i<3; ++i)
            mag[i] = std::sqrt(modelView(i,0) * modelView(i,0) + modelView(i,1) * modelView(i,1) + modelView(i,2) * modelView(i,2));

        for (int i=0; i<3; ++i)
            for (int j=0; j<3; ++j)
                modelView(i,j) = 0;
        modelView(0,0) = mag[0];
        modelView(1,1) = mag[1];
        modelView(2,2) = mag[2];

        return modelView;
    }
}
