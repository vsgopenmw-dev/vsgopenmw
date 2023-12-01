#ifndef VSGOPENMW_MWRENDER_EFFECT_H
#define VSGOPENMW_MWRENDER_EFFECT_H

#include <optional>

#include "../mwworld/ptr.hpp"

namespace ESM
{
    class RefId;
}

namespace MWRender
{
    void addEffect(const MWWorld::Ptr& ptr, const ESM::RefId& effect, int magicEffectId = -1, bool loop = false, const std::string& bone = {}, const std::string& overrideTexture = {});
    void removeEffect(const MWWorld::Ptr& ptr, std::optional<int> effectId = {});
}

#endif
