#include "shadow.hpp"

#include <components/settings/settings.hpp>

#include "mask.hpp"

namespace MWRender
{
    Shadow::Shadow(vsg::Context &ctx)
    {
        auto cat = "Shadows";
        if (Settings::Manager::getBool("enable shadows", cat))
        {
            shadow = std::make_unique<View::Shadow>(ctx, Settings::Manager::getInt("number of shadow maps", cat), Settings::Manager::getInt("shadow map resolution", cat));
        }
        mFar = Settings::Manager::getFloat("maximum shadow map distance", cat);
        /*
shadow fade start = 0.9
         */
    }

    Shadow::~Shadow()
    {
    }

    vsg::Descriptor *Shadow::shadowMap()
    {
        if (shadow)
            return shadow->shadowMap();
        return {};
    }

    void Shadow::updateCascades(const vsg::Camera &camera, Pipeline::Scene &data, const vsg::vec3 &lightPos)
    {
        if (shadow)
            shadow->updateCascades(camera, data, lightPos, mFar);
    }
}
