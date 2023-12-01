#include "shadow.hpp"

#include <limits>

#include <components/settings/settings.hpp>
#include <components/pipeline/viewdata.hpp>

#include "mask.hpp"

namespace MWRender
{
    namespace
    {
        auto cat = "Shadows";
    }

    Shadow::Shadow(vsg::Context& ctx, bool supportsDepthClamp)
    {
        if (Settings::Manager::getBool("enable shadows", cat))
        {
            shadow = std::make_unique<View::Shadow>(ctx, Settings::Manager::getInt("number of shadow maps", cat),
                Settings::Manager::getInt("shadow map resolution", cat),
                Settings::Manager::getBool("filter shadow maps", cat),
                supportsDepthClamp,
                Settings::Manager::getBool("use front face culling", cat)
            );
        }
        maxZ = Settings::Manager::getFloat("maximum shadow map distance", cat);
        if (maxZ == 0)
            maxZ = std::numeric_limits<float>::max();
        cascadeSplitLambda = Settings::Manager::getFloat("split point uniform logarithmic ratio", cat);
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
        if (Settings::Manager::getBool("terrain shadows", cat))
            mask |= Mask_Terrain;
        if (/*exterior && */Settings::Manager::getBool("object shadows", cat))
            mask |= Mask_Object;
        if (Settings::Manager::getBool("actor shadows", cat))
            mask |= Mask_Actor;
        if (Settings::Manager::getBool("player shadows", cat))
            mask |= Mask_Player;
        return mask;
    }
}
