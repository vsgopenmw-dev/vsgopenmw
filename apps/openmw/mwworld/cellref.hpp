#ifndef OPENMW_MWWORLD_CELLREF_H
#define OPENMW_MWWORLD_CELLREF_H

#include <components/esm3/cellref.hpp>

namespace ESM
{
    struct ObjectState;
    struct Enchantment;
}

namespace MWWorld
{

    /// \brief Encapsulated variant of ESM::CellRef with change tracking
    class CellRef
    {
    public:

        CellRef (const ESM::CellRef& ref)
            : mCellRef(ref)
        {
        }

        // Note: Currently unused for items in containers
        const ESM::RefNum& getRefNum() const;

        // Set RefNum to its default state.
        void unsetRefNum();
        void setRefNum(ESM::RefNum refNum);

        /// Does the RefNum have a content file?
        bool hasContentFile() const;

        // Id of object being referenced
        ESM::RefId getRefId() const;

        // Pointer to ID of the object being referenced
        //const std::string* getRefIdPtr() const;

        // For doors - true if this door teleports to somewhere else, false
        // if it should open through animation.
        bool getTeleport() const;

        // Teleport location for the door, if this is a teleporting door.
        ESM::Position getDoorDest() const;

        // Destination cell for doors (optional)
        ESM::RefId getDestCell() const;

        // Scale applied to mesh
        float getScale() const;
        void setScale(float scale);

        // The *original* position and rotation as it was given in the Construction Set.
        // Current position and rotation of the object is stored in RefData.
        ESM::Position getPosition() const;
        void setPosition (const ESM::Position& position);

        // Remaining enchantment charge. This could be -1 if the charge was not touched yet (i.e. full).
        float getEnchantmentCharge() const;

        // Remaining enchantment charge rescaled to the supplied maximum charge (such as one of the enchantment).
        float getNormalizedEnchantmentCharge(const ESM::Enchantment& ench) const;

        void setEnchantmentCharge(float charge);

        // For weapon or armor, this is the remaining item health.
        // For tools (lockpicks, probes, repair hammer) it is the remaining uses.
        // If this returns int(-1) it means full health.
        int getCharge() const;
        float getChargeFloat() const; // Implemented as union with int charge
        void setCharge(int charge);
        void setChargeFloat(float charge);
        void applyChargeRemainderToBeSubtracted(float chargeRemainder); // Stores remainders and applies if > 1

        // The NPC that owns this object (and will get angry if you steal it)
        ESM::RefId getOwner() const;
        void setOwner(const ESM::RefId& owner);

        // Name of a global variable. If the global variable is set to '1', using the object is temporarily allowed
        // even if it has an Owner field.
        // Used by bed rent scripts to allow the player to use the bed for the duration of the rent.
        std::string getGlobalVariable() const;

        void resetGlobalVariable();

        // ID of creature trapped in this soul gem
        ESM::RefId getSoul() const;
        void setSoul(const ESM::RefId& soul);

        // The faction that owns this object (and will get angry if
        // you take it and are not a faction member)
        ESM::RefId getFaction() const;
        void setFaction (const ESM::RefId& faction);

        // PC faction rank required to use the item. Sometimes is -1, which means "any rank".
        void setFactionRank(int factionRank);
        int getFactionRank() const;

        // Lock level for doors and containers
        // Positive for a locked door. 0 for a door that was never locked.
        // For an unlocked door, it is set to -(previous locklevel)
        int getLockLevel() const;
        void setLockLevel(int lockLevel);
        bool isLocked() const { return mCellRef.mIsLocked; }
        void setLocked(bool locked) { mCellRef.mIsLocked = locked; }
        void lock(int lockLevel);
        void unlock();
         // Key and trap ID names, if any
        ESM::RefId getKey() const;
        void setKey(const ESM::RefId& key);
        ESM::RefId getTrap() const;
        void setTrap(const ESM::RefId& trap);

        // This is 5 for Gold_005 references, 100 for Gold_100 and so on.
        int getGoldValue() const;
        void setGoldValue(int value);

        // Write the content of this CellRef into the given ObjectState
        void writeState (ESM::ObjectState& state) const;

        // Has this CellRef changed since it was originally loaded?
        bool hasChanged() const;

        ESM::RefNum getOrAssignRefNum(ESM::RefNum& lastAssignedRefNum);
    private:
        bool mChanged{};
        ESM::CellRef mCellRef;
    };

}

#endif
