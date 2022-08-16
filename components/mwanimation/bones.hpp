#ifndef VSGOPENMW_MWANIMATION_BONES_H
#define VSGOPENMW_MWANIMATION_BONES_H

#include <components/misc/strings/lower.hpp>
#include <components/animation/bones.hpp>

namespace MWAnim
{
    template <typename... Args>
    inline Anim::Bones Bones(Args&&... args)
    {
        return {Misc::StringUtils::lowerCase, std::forward<Args&&>(args)...};
    }
}

#endif
