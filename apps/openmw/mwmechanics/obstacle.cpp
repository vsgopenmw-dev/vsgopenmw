#include "obstacle.hpp"

#include <array>
#include <span>

#include <components/detournavigator/agentbounds.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/mwanimation/position.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"

#include "movement.hpp"

namespace MWMechanics
{
    namespace
    {
        // NOTE: determined empirically but probably need further tweaking
        constexpr float distanceSameSpot = 0.5f;
        constexpr float durationSameSpot = 1.5f;
        constexpr float durationToEvade = 1;

        struct EvadeDirection
        {
            float mMovementX;
            float mMovementY;
            MWWorld::MovementDirectionFlag mRequiredAnimation;
        };

        constexpr EvadeDirection evadeDirections[] = {
            { 1.0f, 1.0f, MWWorld::MovementDirectionFlag_Forward }, // move to right and forward
            { 1.0f, 0.0f, MWWorld::MovementDirectionFlag_Right }, // move to right
            { 1.0f, -1.0f, MWWorld::MovementDirectionFlag_Back }, // move to right and backwards
            { 0.0f, -1.0f, MWWorld::MovementDirectionFlag_Back }, // move backwards
            { -1.0f, -1.0f, MWWorld::MovementDirectionFlag_Back }, // move to left and backwards
            { -1.0f, 0.0f, MWWorld::MovementDirectionFlag_Left }, // move to left
            { -1.0f, 1.0f, MWWorld::MovementDirectionFlag_Forward }, // move to left and forward
        };
    }

    bool proximityToDoor(const MWWorld::Ptr& actor, float minDist)
    {
        if (getNearbyDoor(actor, minDist).isEmpty())
            return false;
        else
            return true;
    }

    const MWWorld::Ptr getNearbyDoor(const MWWorld::Ptr& actor, float minDist)
    {
        MWWorld::CellStore* cell = actor.getCell();

        // Check all the doors in this cell
        const MWWorld::CellRefList<ESM::Door>& doors = cell->getReadOnlyDoors();
        osg::Vec3f pos(actor.getRefData().getPosition().asVec3());
        pos.z() = 0;

        auto dir = MWAnim::forward(actor.getRefData().getPosition());
        osg::Vec3f actorDir = { dir.x, dir.y, dir.z };

        for (const auto& ref : doors.mList)
        {
            osg::Vec3f doorPos(ref.mData.getPosition().asVec3());

            // FIXME: cast
            const MWWorld::Ptr doorPtr
                = MWWorld::Ptr(&const_cast<MWWorld::LiveCellRef<ESM::Door>&>(ref), actor.getCell());

            const auto doorState = doorPtr.getClass().getDoorState(doorPtr);
            float doorRot = ref.mData.getPosition().rot[2] - doorPtr.getCellRef().getPosition().rot[2];

            if (doorState != MWWorld::DoorState::Idle || doorRot != 0)
                continue; // the door is already opened/opening

            doorPos.z() = 0;

            float angle = std::acos(actorDir * (doorPos - pos) / (actorDir.length() * (doorPos - pos).length()));

            // Allow 60 degrees angle between actor and door
            if (angle < -osg::PI / 3 || angle > osg::PI / 3)
                continue;

            // Door is not close enough
            if ((pos - doorPos).length2() > minDist * minDist)
                continue;

            return doorPtr; // found, stop searching
        }

        return MWWorld::Ptr(); // none found
    }

    bool isAreaOccupiedByOtherActor(const MWWorld::ConstPtr& actor, const osg::Vec3f& destination, bool ignorePlayer,
        std::vector<MWWorld::Ptr>* occupyingActors)
    {
        const auto world = MWBase::Environment::get().getWorld();
        const osg::Vec3f halfExtents = world->getPathfindingAgentBounds(actor).mHalfExtents;
        const auto maxHalfExtent = std::max(halfExtents.x(), std::max(halfExtents.y(), halfExtents.z()));
        if (ignorePlayer)
        {
            const std::array ignore{ actor, world->getPlayerConstPtr() };
            return world->isAreaOccupiedByOtherActor(destination, 2 * maxHalfExtent, ignore, occupyingActors);
        }
        const std::array ignore{ actor };
        return world->isAreaOccupiedByOtherActor(destination, 2 * maxHalfExtent, ignore, occupyingActors);
    }

    ObstacleCheck::ObstacleCheck()
        : mEvadeDirectionIndex(std::size(evadeDirections) - 1)
    {
    }

    void ObstacleCheck::clear()
    {
        mWalkState = WalkState::Initial;
    }

    bool ObstacleCheck::isEvading() const
    {
        return mWalkState == WalkState::Evade;
    }

    /*
     * input   - actor, duration (time since last check)
     * output  - true if evasive action needs to be taken
     *
     * Walking state transitions (player greeting check not shown):
     *
     * Initial ----> Norm  <--------> CheckStuck -------> Evade ---+
     *               ^ ^ | f             ^   |       t    ^   |    |
     *               | | |               |   |            |   |    |
     *               | +-+               +---+            +---+    | u
     *               | any                < t              < u     |
     *               +---------------------------------------------+
     *
     * f = one reaction time
     * t = how long before considered stuck
     * u = how long to move sideways
     *
     */
    void ObstacleCheck::update(const MWWorld::Ptr& actor, const osg::Vec3f& destination, float duration,
        MWWorld::MovementDirectionFlags supportedMovementDirection)
    {
        const auto position = actor.getRefData().getPosition().asVec3();

        if (mWalkState == WalkState::Initial)
        {
            mWalkState = WalkState::Norm;
            mStateDuration = 0;
            mPrev = position;
            mInitialDistance = (destination - position).length();
            mDestination = destination;
            return;
        }

        if (mWalkState != WalkState::Evade)
        {
            if (mDestination != destination)
            {
                mInitialDistance = (destination - mPrev).length();
                mDestination = destination;
            }

            const float distSameSpot = distanceSameSpot * actor.getClass().getCurrentSpeed(actor) * duration;
            const float prevDistance = (destination - mPrev).length();
            const float currentDistance = (destination - position).length();
            const float movedDistance = prevDistance - currentDistance;
            const float movedFromInitialDistance = mInitialDistance - currentDistance;

            mPrev = position;

            if (movedDistance >= distSameSpot && movedFromInitialDistance >= distSameSpot)
            {
                mWalkState = WalkState::Norm;
                mStateDuration = 0;
                return;
            }

            if (mWalkState == WalkState::Norm)
            {
                mWalkState = WalkState::CheckStuck;
                mStateDuration = duration;
                mInitialDistance = (destination - position).length();
                return;
            }

            mStateDuration += duration;
            if (mStateDuration < durationSameSpot)
            {
                return;
            }

            mWalkState = WalkState::Evade;
            mStateDuration = 0;
            std::size_t newEvadeDirectionIndex = mEvadeDirectionIndex;
            do
            {
                ++newEvadeDirectionIndex;
                if (newEvadeDirectionIndex == std::size(evadeDirections))
                    newEvadeDirectionIndex = 0;
                if ((evadeDirections[newEvadeDirectionIndex].mRequiredAnimation & supportedMovementDirection) != 0)
                    break;
            } while (mEvadeDirectionIndex != newEvadeDirectionIndex);
            return;
        }

        mStateDuration += duration;
        if (mStateDuration >= durationToEvade)
        {
            // tried to evade, assume all is ok and start again
            mWalkState = WalkState::Norm;
            mStateDuration = 0;
            mPrev = position;
        }
    }

    void ObstacleCheck::takeEvasiveAction(MWMechanics::Movement& actorMovement) const
    {
        actorMovement.mPosition[0] = evadeDirections[mEvadeDirectionIndex].mMovementX;
        actorMovement.mPosition[1] = evadeDirections[mEvadeDirectionIndex].mMovementY;
    }
}
