#ifndef VSGOPENMW_MWANIMATION_GROUPS_H
#define VSGOPENMW_MWANIMATION_GROUPS_H

#include "play.hpp"

namespace MWAnim
{
    static const std::map<std::string, uint32_t> groups = {{"Bip01 Spine1",1}, {"Bip01 L Clavicle",2}, {"Bip01 R Clavicle",3}};

    enum BoneGroup {
        BoneGroup_LowerBody = 0,
        BoneGroup_Torso,
        BoneGroup_LeftArm,
        BoneGroup_RightArm
    };

    enum BlendMask {
        BlendMask_LowerBody = 1<<0,
        BlendMask_Torso = 1<<1,
        BlendMask_LeftArm = 1<<2,
        BlendMask_RightArm = 1<<3,

        BlendMask_UpperBody = BlendMask_Torso | BlendMask_LeftArm | BlendMask_RightArm,

        BlendMask_All = BlendMask_LowerBody | BlendMask_UpperBody
    };

    /* This is the number of *discrete* blend masks. */
    static constexpr size_t sNumBlendMasks = 4;

    struct Priority : public Play::Priority
    {
        Priority(int val)
            : Play::Priority(sNumBlendMasks, val) {}
    };
}

#endif
