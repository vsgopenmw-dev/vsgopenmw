#include "cellref.hpp"

#include <iostream>

#include <components/esm3/objectstate.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm/util.hpp>

#include <apps/openmw/mwmechanics/spellutil.hpp>

namespace MWWorld
{

    const ESM::RefNum& CellRef::getRefNum() const
    {
        return mCellRef.mRefNum;
    }

    bool CellRef::hasContentFile() const
    {
        return mCellRef.mRefNum.hasContentFile();
    }

    ESM::RefId CellRef::getRefId() const
    {
        return mCellRef.mRefID;
    }

    /*
    const std::string* CellRef::getRefIdPtr() const
    {
        return &mCellRef.mRefID;
    }
*/
    bool CellRef::getTeleport() const
    {
        return mCellRef.mTeleport;
    }

    ESM::Position CellRef::getDoorDest() const
    {
        return mCellRef.mDoorDest;
    }

    ESM::RefId CellRef::getDestCell() const
    {
        const auto& ref = mCellRef;
        if (!ref.mDestCell.empty())
        {
            return ESM::RefId::stringRefId(ref.mDestCell);
        }
        else
        {
            const auto cellPos = ESM::positionToExteriorCellLocation(ref.mDoorDest.pos[0], ref.mDoorDest.pos[1]);
            return ESM::RefId::esm3ExteriorCell(cellPos.mX, cellPos.mY);
        }
    }

    float CellRef::getScale() const
    {
        return mCellRef.mScale;
    }

    void CellRef::setScale(float scale)
    {
        if (scale != mCellRef.mScale)
        {
            mChanged = true;
            mCellRef.mScale = scale;
        }
    }

    ESM::Position CellRef::getPosition() const
    {
        return mCellRef.mPos;
    }

    void CellRef::setPosition(const ESM::Position &position)
    {
        mChanged = true;
        mCellRef.mPos = position;
    }

    float CellRef::getEnchantmentCharge() const
    {
        return mCellRef.mEnchantmentCharge;
    }

    // TODO: doesn't this functionality belong elsewhere? CellRef is only an Encapsulated variant of ESM::CellRef with change tracking.
    float CellRef::getNormalizedEnchantmentCharge(const ESM::Enchantment& enchantment) const
    {
        const int maxCharge = MWMechanics::getEnchantmentCharge(enchantment);
        if (maxCharge == 0)
        {
            return 0;
        }
        else if (getEnchantmentCharge() == -1)
        {
            return 1;
        }
        else
        {
            return getEnchantmentCharge() / static_cast<float>(maxCharge);
        }
    }

    void CellRef::setEnchantmentCharge(float charge)
    {
        if (charge != mCellRef.mEnchantmentCharge)
        {
            mChanged = true;
            mCellRef.mEnchantmentCharge = charge;
        }
    }

    int CellRef::getCharge() const
    {
        return mCellRef.mChargeInt;
    }

    void CellRef::setCharge(int charge)
    {
        if (charge != mCellRef.mChargeInt)
        {
            mChanged = true;
            mCellRef.mChargeInt = charge;
        }
    }

    // TODO: doesn't this functionality belong elsewhere? CellRef is only an Encapsulated variant of ESM::CellRef with change tracking.
    void CellRef::applyChargeRemainderToBeSubtracted(float chargeRemainder)
    {
        mCellRef.mChargeIntRemainder += std::abs(chargeRemainder);
        if (mCellRef.mChargeIntRemainder > 1.0f)
        {
            float newChargeRemainder = (mCellRef.mChargeIntRemainder - std::floor(mCellRef.mChargeIntRemainder));
            if (mCellRef.mChargeInt <= static_cast<int>(mCellRef.mChargeIntRemainder))
            {
                mCellRef.mChargeInt = 0;
            }
            else
            {
                mCellRef.mChargeInt -= static_cast<int>(mCellRef.mChargeIntRemainder);
            }
            mCellRef.mChargeIntRemainder = newChargeRemainder;
        }
    }

    float CellRef::getChargeFloat() const
    {
        return mCellRef.mChargeFloat;
    }

    void CellRef::setChargeFloat(float charge)
    {
        if (charge != mCellRef.mChargeFloat)
        {
            mChanged = true;
            mCellRef.mChargeFloat = charge;
        }
    }

    ESM::RefId CellRef::getOwner() const
    {
        return mCellRef.mOwner;
    }

    std::string CellRef::getGlobalVariable() const
    {
        return mCellRef.mGlobalVariable;
    }

    void CellRef::resetGlobalVariable()
    {
        if (!mCellRef.mGlobalVariable.empty())
        {
            mChanged = true;
            mCellRef.mGlobalVariable.erase();
        }
    }

    void CellRef::setFactionRank(int factionRank)
    {
        if (factionRank != mCellRef.mFactionRank)
        {
            mChanged = true;
            mCellRef.mFactionRank = factionRank;
        }
    }

    int CellRef::getFactionRank() const
    {
        return mCellRef.mFactionRank;
    }

    void CellRef::setOwner(const ESM::RefId &owner)
    {
        if (owner != mCellRef.mOwner)
        {
            mChanged = true;
            mCellRef.mOwner = owner;
        }
    }

    ESM::RefId CellRef::getSoul() const
    {
        return mCellRef.mSoul;
    }

    void CellRef::setSoul(const ESM::RefId &soul)
    {
        if (soul != mCellRef.mSoul)
        {
            mChanged = true;
            mCellRef.mSoul = soul;
        }
    }

    ESM::RefId CellRef::getFaction() const
    {
        return mCellRef.mFaction;
    }

    void CellRef::setFaction(const ESM::RefId &faction)
    {
        if (faction != mCellRef.mFaction)
        {
            mChanged = true;
            mCellRef.mFaction = faction;
        }
    }

    int CellRef::getLockLevel() const
    {
        return mCellRef.mLockLevel;
    }

    void CellRef::setLockLevel(int lockLevel)
    {
        if (lockLevel != mCellRef.mLockLevel)
        {
            mChanged = true;
            mCellRef.mLockLevel = lockLevel;
        }
    }

    void CellRef::lock(int lockLevel)
    {
        setLockLevel(lockLevel);
        setLocked(true);
    }

    void CellRef::unlock()
    {
        setLockLevel(-getLockLevel());
        setLocked(false);
    }

    ESM::RefId CellRef::getKey() const
    {
        return mCellRef.mKey;
    }

    ESM::RefId CellRef::getTrap() const
    {
        return mCellRef.mTrap;
    }

    void CellRef::setKey(const ESM::RefId& key)
    {
        if (key != mCellRef.mKey)
        {
            mChanged = true;
            mCellRef.mKey = key;
        }
    }

    void CellRef::setTrap(const ESM::RefId& trap)
    {
        if (trap != mCellRef.mTrap)
        {
            mChanged = true;
            mCellRef.mTrap = trap;
        }
    }

    int CellRef::getGoldValue() const
    {
        return mCellRef.mGoldValue;
    }

    void CellRef::setGoldValue(int value)
    {
        if (value != mCellRef.mGoldValue)
        {
            mChanged = true;
            mCellRef.mGoldValue = value;
        }
    }

    void CellRef::writeState(ESM::ObjectState &state) const
    {
        state.mRef = mCellRef;
    }

    bool CellRef::hasChanged() const
    {
        return mChanged;
    }

    void CellRef::unsetRefNum()
    {
        setRefNum({});
    }

    void CellRef::setRefNum(ESM::RefNum refNum)
    {
        mCellRef.mRefNum = refNum;
    }

    ESM::RefNum CellRef::getOrAssignRefNum(ESM::RefNum& lastAssignedRefNum)
    {
        auto& refNum = mCellRef.mRefNum;
        if (!refNum.isSet())
        {
            // Generated RefNums have negative mContentFile
            assert(lastAssignedRefNum.mContentFile < 0);
            lastAssignedRefNum.mIndex++;
            if (lastAssignedRefNum.mIndex == 0) // mIndex overflow, so mContentFile should be changed
            {
                if (lastAssignedRefNum.mContentFile > std::numeric_limits<int32_t>::min())
                    lastAssignedRefNum.mContentFile--;
                else
                    std::cerr << "RefNum counter overflow in CellRef::getOrAssignRefNum" << std::endl;
            }
            refNum = lastAssignedRefNum;
            mChanged = true;
        }
        return refNum;
    }

}
