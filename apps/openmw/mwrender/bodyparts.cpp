#include "bodyparts.hpp"

#include <iostream>

using Store = MWWorld::Store<ESM::BodyPart>;

namespace
{
std::string getModel(const std::string &bodyPart, const Store &store)
{
    if (auto bp = store.search(bodyPart))
        return bp->mModel;
    else
        std::cerr << "Failed to load body part '" << bodyPart << "'" << std::endl;
    return{};
}

std::string getVampireHead(const std::string& race, bool female, const Store &store)
{
    static std::map <std::pair<std::string,int>, const ESM::BodyPart* > sVampireMapping;

    std::pair<std::string, int> thisCombination = std::make_pair(race, int(female));

    if (sVampireMapping.find(thisCombination) == sVampireMapping.end())
    {
        for (const ESM::BodyPart& bodypart : store)
        {
            if (!bodypart.mData.mVampire)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;
            if (bodypart.mData.mPart != ESM::BodyPart::MP_Head)
                continue;
            if (female != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
                continue;
            if (!Misc::StringUtils::ciEqual(bodypart.mRace, race))
                continue;
            sVampireMapping[thisCombination] = &bodypart;
        }
    }

    sVampireMapping.emplace(thisCombination, nullptr);

    const ESM::BodyPart* bodyPart = sVampireMapping[thisCombination];
    if (!bodyPart)
        return std::string();
    return bodyPart->mModel;
}

bool isFirstPersonPart(const ESM::BodyPart* bodypart)
{
    return bodypart->mId.size() >= 3 && bodypart->mId.substr(bodypart->mId.size()-3, 3) == "1st";
}

bool isFemalePart(const ESM::BodyPart* bodypart)
{
    return bodypart->mData.mFlags & ESM::BodyPart::BPF_Female;
}
}

namespace MWRender
{
// Remember body parts so we only have to search through the store once for each race/gender/viewmode combination
typedef std::map< std::pair<std::string,int>,std::vector<const ESM::BodyPart*> > RaceMapping;
static RaceMapping sRaceMapping;

const std::vector<const ESM::BodyPart *> &getBodyParts(const std::string &race, bool female, bool firstPerson, bool werewolf, const Store &store)
{
    static const int Flag_FirstPerson = 1<<1;
    static const int Flag_Female      = 1<<0;

    int flags = (werewolf ? -1 : 0);
    if(female)
        flags |= Flag_Female;
    if(firstPerson)
        flags |= Flag_FirstPerson;

    RaceMapping::iterator found = sRaceMapping.find(std::make_pair(race, flags));
    if (found != sRaceMapping.end())
        return found->second;
    else
    {
        std::vector<const ESM::BodyPart*>& parts = sRaceMapping[std::make_pair(race, flags)];

        typedef std::multimap<ESM::BodyPart::MeshPart,ESM::PartReferenceType> BodyPartMapType;
        static const BodyPartMapType sBodyPartMap =
        {
            {ESM::BodyPart::MP_Neck, ESM::PRT_Neck},
            {ESM::BodyPart::MP_Chest, ESM::PRT_Cuirass},
            {ESM::BodyPart::MP_Groin, ESM::PRT_Groin},
            {ESM::BodyPart::MP_Hand, ESM::PRT_RHand},
            {ESM::BodyPart::MP_Hand, ESM::PRT_LHand},
            {ESM::BodyPart::MP_Wrist, ESM::PRT_RWrist},
            {ESM::BodyPart::MP_Wrist, ESM::PRT_LWrist},
            {ESM::BodyPart::MP_Forearm, ESM::PRT_RForearm},
            {ESM::BodyPart::MP_Forearm, ESM::PRT_LForearm},
            {ESM::BodyPart::MP_Upperarm, ESM::PRT_RUpperarm},
            {ESM::BodyPart::MP_Upperarm, ESM::PRT_LUpperarm},
            {ESM::BodyPart::MP_Foot, ESM::PRT_RFoot},
            {ESM::BodyPart::MP_Foot, ESM::PRT_LFoot},
            {ESM::BodyPart::MP_Ankle, ESM::PRT_RAnkle},
            {ESM::BodyPart::MP_Ankle, ESM::PRT_LAnkle},
            {ESM::BodyPart::MP_Knee, ESM::PRT_RKnee},
            {ESM::BodyPart::MP_Knee, ESM::PRT_LKnee},
            {ESM::BodyPart::MP_Upperleg, ESM::PRT_RLeg},
            {ESM::BodyPart::MP_Upperleg, ESM::PRT_LLeg},
            {ESM::BodyPart::MP_Tail, ESM::PRT_Tail}
        };

        parts.resize(ESM::PRT_Count, nullptr);

        if (werewolf)
            return parts;

        for(const ESM::BodyPart& bodypart : store)
        {
            if (bodypart.mData.mFlags & ESM::BodyPart::BPF_NotPlayable)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;

            if (!Misc::StringUtils::ciEqual(bodypart.mRace, race))
                continue;

            bool partFirstPerson = isFirstPersonPart(&bodypart);

            bool isHand = bodypart.mData.mPart == ESM::BodyPart::MP_Hand ||
                                    bodypart.mData.mPart == ESM::BodyPart::MP_Wrist ||
                                    bodypart.mData.mPart == ESM::BodyPart::MP_Forearm ||
                                    bodypart.mData.mPart == ESM::BodyPart::MP_Upperarm;

            bool isSameGender = isFemalePart(&bodypart) == female;

            /* A fallback for the arms if 1st person is missing:
             1. Try to use 3d person skin for same gender
             2. Try to use 1st person skin for male, if female == true
             3. Try to use 3d person skin for male, if female == true

             A fallback in another cases: allow to use male bodyparts, if female == true
            */
            if (firstPerson && isHand && !partFirstPerson)
            {
                // Allow 3rd person skins as a fallback for the arms if 1st person is missing
                BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                {
                    // If we have no fallback bodypart now and bodypart is for same gender (1)
                    if(!parts[bIt->second] && isSameGender)
                       parts[bIt->second] = &bodypart;

                    // If we have fallback bodypart for other gender and found fallback for current gender (1)
                    else if(isSameGender && isFemalePart(parts[bIt->second]) != female)
                       parts[bIt->second] = &bodypart;

                    // If we have no fallback bodypart and searching for female bodyparts (3)
                    else if(!parts[bIt->second] && female)
                       parts[bIt->second] = &bodypart;

                    ++bIt;
                }

                continue;
            }

            // Don't allow to use podyparts for a different view
            if (partFirstPerson != firstPerson)
                continue;

            if (female && !isFemalePart(&bodypart))
            {
                // Allow male parts as fallback for females if female parts are missing
                BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                {
                    // If we have no fallback bodypart now
                    if(!parts[bIt->second])
                        parts[bIt->second] = &bodypart;

                    // If we have 3d person fallback bodypart for hand and 1st person fallback found (2)
                    else if(isHand && !isFirstPersonPart(parts[bIt->second]) && partFirstPerson)
                        parts[bIt->second] = &bodypart;

                    ++bIt;
                }

                continue;
            }

            // Don't allow to use podyparts for another gender
            if (female != isFemalePart(&bodypart))
                continue;

            // Use properly found bodypart, replacing fallbacks
            BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
            while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
            {
                parts[bIt->second] = &bodypart;
                ++bIt;
            }
        }
        return parts;
    }
}

std::string getHeadModel(bool werewolf, bool vampire, bool female, const std::string &race, const std::string &bodypartName, const Store &store)
{
    if (vampire && !werewolf)
    {
        const std::string& vampireHead = getVampireHead(race, female, store);
        if (!vampireHead.empty())
            return vampireHead;
    }
    else if (werewolf)
        return getModel("WerewolfHead", store);
    if (!bodypartName.empty())
        return getModel(bodypartName, store);
    return {};
}

std::string getHairModel(bool werewolf, const std::string &hairName, const Store &store)
{
    if (werewolf)
        return getModel("WerewolfHair", store);
    if (!hairName.empty())
        return getModel(hairName, store);
    return {};
}

const ESM::BodyPart *getBodyPart(bool firstPerson, const std::string &base, const Store &store)
{
    const char *ext = firstPerson ? ".1st" : "";
    auto bodypart = store.search(base+ext);
    if(!bodypart && firstPerson)
    {
        bodypart = store.search(base);
        if(bodypart && !(bodypart->mData.mPart == ESM::BodyPart::MP_Hand ||
                         bodypart->mData.mPart == ESM::BodyPart::MP_Wrist ||
                         bodypart->mData.mPart == ESM::BodyPart::MP_Forearm ||
                         bodypart->mData.mPart == ESM::BodyPart::MP_Upperarm))
            bodypart = nullptr;
    }
    else if (!bodypart)
        std::cerr << "Failed to find body part '" << base << "'"  << std::endl;
    return bodypart;
}

const ESM::BodyPart *getBodyPart(bool firstPerson, bool female, const ESM::PartReference &part, const Store &store)
{
    const ESM::BodyPart *bodypart = nullptr;
    if(female && !part.mFemale.empty())
        bodypart = getBodyPart(firstPerson, part.mFemale, store);
    if(!bodypart && !part.mMale.empty())
        bodypart = getBodyPart(firstPerson, part.mMale, store);
    return bodypart;
}

}
