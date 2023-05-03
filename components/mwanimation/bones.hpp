#ifndef VSGOPENMW_MWANIMATION_BONES_H
#define VSGOPENMW_MWANIMATION_BONES_H

#include <components/animation/bones.hpp>
#include <components/misc/strings/lower.hpp>

namespace MWAnim
{
    template <typename... Args>
    inline Anim::Bones Bones(Args&&... args)
    {
        return { [](const std::string& s) -> std::string { return Misc::StringUtils::lowerCase(s); },
            std::forward<Args&&>(args)... };
    }
}

#endif
