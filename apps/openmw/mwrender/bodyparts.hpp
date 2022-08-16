#ifndef VSGOPENMW_MWRENDER_BODYPARTS_H
#define VSGOPENMW_MWRENDER_BODYPARTS_H

#include "../mwworld/store.hpp"

namespace MWRender
{
    /// Get a list of body parts that may be used by an NPC of given race and gender.
    /// @note This is a fixed size list, one list item for each ESM::PartReferenceType, may contain nullptr body parts.
    const std::vector<const ESM::BodyPart *> &getBodyParts(const std::string &race, bool female, bool firstPerson, bool werewolf, const MWWorld::Store<ESM::BodyPart> &store);

    std::string getHeadModel(bool werewolf, bool vampire, bool female, const std::string &race, const std::string &bodypartName, const MWWorld::Store<ESM::BodyPart> &store);

    std::string getHairModel(bool werewolf, const std::string &bodypartName, const MWWorld::Store<ESM::BodyPart> &store);

    const ESM::BodyPart *getBodyPart(bool firstPerson, bool female, const ESM::PartReference &part, const MWWorld::Store<ESM::BodyPart> &store);
}

#endif
