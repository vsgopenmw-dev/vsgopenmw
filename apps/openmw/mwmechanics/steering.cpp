#include "steering.hpp"

#include <components/misc/mathutil.hpp>
#include <components/settings/values.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "movement.hpp"

namespace MWMechanics
{

    bool smoothTurn(const MWWorld::Ptr& actor, float dt, float targetAngleRadians, int axis, float epsilonRadians)
    {
        MWMechanics::Movement& movement = actor.getClass().getMovementSettings(actor);
        float diff = Misc::normalizeAngle(targetAngleRadians - actor.getRefData().getPosition().rot[axis]);
        float absDiff = std::abs(diff);

        // The turning animation actually moves you slightly, so the angle will be wrong again.
        // Use epsilon to prevent jerkiness.
        if (absDiff < epsilonRadians)
            return true;

        float limit
            = getAngularVelocity(actor.getClass().getMaxSpeed(actor)) * dt;
        if (Settings::game().mSmoothMovement)
            limit *= std::min(absDiff / osg::PI + 0.1, 0.5);

        if (absDiff > limit)
            diff = osg::sign(diff) * limit;

        movement.mRotation[axis] = diff;
        return false;
    }

    bool zTurn(const MWWorld::Ptr& actor, float duration, float targetAngleRadians, float epsilonRadians)
    {
        return smoothTurn(actor, duration, targetAngleRadians, 2, epsilonRadians);
    }

}
