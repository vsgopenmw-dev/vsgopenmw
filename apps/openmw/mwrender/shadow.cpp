#include "shadow.hpp"

#include <components/settings/settings.hpp>

#include "mask.hpp"

namespace MWRender
{
    namespace
    {
        auto cat = "Shadows";
    }
    Shadow::Shadow(vsg::Context& ctx)
    {
        if (Settings::Manager::getBool("enable shadows", cat))
        {
            shadow = std::make_unique<View::Shadow>(ctx, Settings::Manager::getInt("number of shadow maps", cat),
                Settings::Manager::getInt("shadow map resolution", cat));
        }
        mFar = Settings::Manager::getFloat("maximum shadow map distance", cat);
        /*
shadow fade start = 0.9
         */
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
