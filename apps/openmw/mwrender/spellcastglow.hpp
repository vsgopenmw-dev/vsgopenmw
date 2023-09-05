#ifndef VSGOPENMW_MWRENDER_SPELLCASTGLOW_H
#define VSGOPENMW_MWRENDER_SPELLCASTGLOW_H

#include <components/esm3/loadmgef.hpp>

namespace MWAnim
{
    class Object;
}
namespace MWRender
{
    void addSpellCastGlow(MWAnim::Object* object, const ESM::MagicEffect& effect, float duration = 1.5);
}

#endif
