#ifndef VSGOPENMW_MWRENDER_SHADOW_H
#define VSGOPENMW_MWRENDER_SHADOW_H

#include <components/view/shadow.hpp>

namespace MWRender
{
    /*
     * Optionally configures shadows.
     */
    class Shadow
    {
    public:
        Shadow(vsg::Context& ctx, bool supportsDepthClamp/*, Settings::Manager&*/);
        ~Shadow();
        void updateCascades(const vsg::Camera& camera, Pipeline::Data::Scene& data, const vsg::vec3& lightPos);
        vsg::Mask viewMask() const;
        vsg::Descriptor* shadowMap();
        std::unique_ptr<View::Shadow> shadow;
        float maxZ;
        float cascadeSplitLambda;
    };
}

#endif
