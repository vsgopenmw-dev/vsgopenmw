#ifndef VSGOPENMW_MWANIMATION_PARTBONES_H
#define VSGOPENMW_MWANIMATION_PARTBONES_H

#include <map>

#include <components/esm3/loadarmo.hpp>

namespace MWAnim
{
    static const std::map<ESM::PartReferenceType, std::string> sPartBones = {
        {ESM::PRT_Head, "Head"},
        {ESM::PRT_Hair, "Head"}, // note it uses "Head" as attach bone, but "Hair" as filter
        {ESM::PRT_Neck, "Neck"},
        {ESM::PRT_Cuirass, "Chest"},
        {ESM::PRT_Groin, "Groin"},
        {ESM::PRT_Skirt, "Groin"},
        {ESM::PRT_RHand, "Right Hand"},
        {ESM::PRT_LHand, "Left Hand"},
        {ESM::PRT_RWrist, "Right Wrist"},
        {ESM::PRT_LWrist, "Left Wrist"},
        {ESM::PRT_Shield, "Shield Bone"},
        {ESM::PRT_RForearm, "Right Forearm"},
        {ESM::PRT_LForearm, "Left Forearm"},
        {ESM::PRT_RUpperarm, "Right Upper Arm"},
        {ESM::PRT_LUpperarm, "Left Upper Arm"},
        {ESM::PRT_RFoot, "Right Foot"},
        {ESM::PRT_LFoot, "Left Foot"},
        {ESM::PRT_RAnkle, "Right Ankle"},
        {ESM::PRT_LAnkle, "Left Ankle"},
        {ESM::PRT_RKnee, "Right Knee"},
        {ESM::PRT_LKnee, "Left Knee"},
        {ESM::PRT_RLeg, "Right Upper Leg"},
        {ESM::PRT_LLeg, "Left Upper Leg"},
        {ESM::PRT_RPauldron, "Right Clavicle"},
        {ESM::PRT_LPauldron, "Left Clavicle"},
        {ESM::PRT_Weapon, "Weapon Bone"}, // Fallback. The real node name depends on the current weapon type.
        {ESM::PRT_Tail, "Tail"}};
}

#endif
