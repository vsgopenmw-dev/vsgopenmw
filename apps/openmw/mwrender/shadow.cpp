#include "shadow.hpp"

#include <limits>

#include <components/settings/values.hpp>
#include <components/pipeline/viewdata.hpp>

#include "mask.hpp"

namespace MWRender
{
    Shadow::Shadow(vsg::Context& ctx, bool supportsDepthClamp)
    {
        if (Settings::shadows().mEnableShadows)
        {
            shadow = std::make_unique<View::Shadow>(ctx, Settings::shadows().mNumberOfShadowMaps,
                Settings::shadows().mShadowMapResolution,
                Settings::shadows().mFilterShadowMaps,
                supportsDepthClamp,
                Settings::shadows().mUseFrontFaceCulling
            );
        }
        maxZ = Settings::shadows().mMaximumShadowMapDistance;
        if (maxZ == 0)
            maxZ = std::numeric_limits<float>::max();
        cascadeSplitLambda = Settings::shadows().mSplitPointUniformLogarithmicRatio;
    }

    void Shadow::updateCascades(const vsg::Camera& camera, Pipeline::Data::Scene& data, const vsg::vec3& lightPos)
    {
        if (shadow)
            shadow->updateCascades(camera, data, lightPos, std::min(maxZ, data.zFar), cascadeSplitLambda);
    }

    Shadow::~Shadow() {}

    vsg::Descriptor* Shadow::shadowMap()
    {
        if (shadow)
            return shadow->shadowMap();
        return {};
    }

    vsg::Mask Shadow::viewMask() const
    {
        vsg::Mask mask = vsg::MASK_OFF;
        if (Settings::shadows().mTerrainShadows)
            mask |= Mask_Terrain;
        if (/*exterior && */Settings::shadows().mObjectShadows)
            mask |= Mask_Object;
        if (Settings::shadows().mActorShadows)
            mask |= Mask_Actor;
        if (Settings::shadows().mPlayerShadows)
            mask |= Mask_Player;
        return mask;
    }
}
