#include "camera.hpp"

#include <vsg/viewer/ViewMatrix.h>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/misc/mathutil.hpp>
#include <components/settings/settings.hpp>
#include <components/vsgutil/computetransform.hpp>
#include <components/animation/transform.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/refdata.hpp"

#include "../mwmechanics/drawstate.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwphysics/raycasting.hpp"

#include "player.hpp"

namespace MWRender
{

    Camera::Camera ()
    : mHeightScale(1.f),
      mCollisionType((MWPhysics::CollisionType::CollisionType_Default & ~MWPhysics::CollisionType::CollisionType_Actor) | MWPhysics::CollisionType_CameraOnly),
      mAnimation(nullptr),
      mFirstPersonView(true),
      mMode(Mode::FirstPerson),
      mVanityAllowed(true),
      mDeferredRotationAllowed(true),
      mProcessViewChange(false),
      mHeight(124.f),
      mPitch(0.f),
      mYaw(0.f),
      mRoll(0.f),
      mCameraDistance(0.f),
      mPreferredCameraDistance(0.f),
      mFocalPointCurrentOffset(osg::Vec2d()),
      mFocalPointTargetOffset(osg::Vec2d()),
      mFocalPointTransitionSpeedCoef(1.f),
      mSkipFocalPointTransition(true),
      mPreviousTransitionInfluence(0.f),
      mShowCrosshair(false),
      mDeferredRotation(osg::Vec3f()),
      mDeferredRotationDisabled(false)
    {
    }

    Camera::~Camera()
    {
    }

    osg::Vec3d Camera::calculateTrackedPosition() const
    {
        if (mTrackingPtr.isEmpty())
            return osg::Vec3d();
        //if (usingDoubleTransformInMwrenderObject) vsg::computeTransform(
        auto res = vsgUtil::computePosition(mTrackingPath);
        if (mMode != Mode::FirstPerson)
            res.z += mHeight * mHeightScale;
        return {res.x,res.y,res.z};
    }

    osg::Vec3d Camera::getFocalPointOffset() const
    {
        osg::Vec3d offset;
        offset.x() = mFocalPointCurrentOffset.x() * cos(mYaw);
        offset.y() = mFocalPointCurrentOffset.x() * sin(mYaw);
        offset.z() = mFocalPointCurrentOffset.y();
        return offset;
    }

    void Camera::updateCamera(vsg::LookAt &lookAt)
    {
        auto orient = vsg::quat(mRoll + mExtraRoll, vsg::vec3(0,1,0)) *
                           vsg::quat(mPitch + mExtraPitch, vsg::vec3(1,0,0)) *
                           vsg::quat(mYaw + mExtraYaw, vsg::vec3(0,0,1));
        auto forward = orient * vsg::vec3(0,1,0);
        auto up = orient * vsg::vec3(0,0,1);
        lookAt.eye = toVsg(mPosition);
        lookAt.center = toVsg(mPosition) + vsg::dvec3(forward);
        lookAt.up = vsg::dvec3(up);
    }

    void Camera::update(float duration, bool paused)
    {
        mLockPitch = mLockYaw = false;
        if (mQueuedMode && !(mAnimation && mAnimation->busy))
            setMode(*mQueuedMode);
        if (mProcessViewChange)
            processViewChange();

        if (paused)
            return;

        // only show the crosshair in game mode
        MWBase::WindowManager *wm = MWBase::Environment::get().getWindowManager();
        wm->showCrosshair(!wm->isGuiMode() && mShowCrosshair);

        updateFocalPointOffset(duration);
        updatePosition();
    }

    osg::Vec3d Camera::calculateFirstPersonPosition(const osg::Vec3d& trackedPosition) const
    {
        osg::Vec3d res = trackedPosition;
        osg::Vec2f horizontalOffset = Misc::rotateVec2f(osg::Vec2f(mFirstPersonOffset.x(), mFirstPersonOffset.y()), mYaw);
        res.x() += horizontalOffset.x();
        res.y() += horizontalOffset.y();
        res.z() += mFirstPersonOffset.z();
        return res;
    }

    void Camera::updatePosition()
    {
        mTrackedPosition = calculateTrackedPosition();
        if (mMode == Mode::Static)
            return;
        if (mMode == Mode::FirstPerson)
        {
            mPosition = calculateFirstPersonPosition(mTrackedPosition);
            mCameraDistance = 0;
            return;
        }

        constexpr float cameraObstacleLimit = 5.0f;
        constexpr float focalObstacleLimit = 10.f;

        const auto* rayCasting = MWBase::Environment::get().getWorld()->getRayCasting();

        // Adjust focal point to prevent clipping.
        osg::Vec3d focalOffset = getFocalPointOffset();
        osg::Vec3d focal = mTrackedPosition + focalOffset;
        focalOffset.z() += 10.f;  // Needed to avoid camera clipping through the ceiling because
                                  // character's head can be a bit higher than the collision area.
        float offsetLen = focalOffset.length();
        if (offsetLen > 0)
        {
            MWPhysics::RayCastingResult result = rayCasting->castSphere(focal - focalOffset, focal, focalObstacleLimit, mCollisionType);
            if (result.mHit)
            {
                double adjustmentCoef = -(result.mHitPos + result.mHitNormal * focalObstacleLimit - focal).length() / offsetLen;
                focal += focalOffset * std::max(-1.0, adjustmentCoef);
            }
        }

        // Adjust camera distance.
        mCameraDistance = mPreferredCameraDistance;
        osg::Quat orient =  osg::Quat(mPitch + mExtraPitch, osg::Vec3d(1,0,0)) * osg::Quat(mYaw + mExtraYaw, osg::Vec3d(0,0,1));
        osg::Vec3d offset = orient * osg::Vec3d(0.f, -mCameraDistance, 0.f);
        MWPhysics::RayCastingResult result = rayCasting->castSphere(focal, focal + offset, cameraObstacleLimit, mCollisionType);
        if (result.mHit)
        {
            mCameraDistance = (result.mHitPos + result.mHitNormal * cameraObstacleLimit - focal).length();
            offset = orient * osg::Vec3d(0.f, -mCameraDistance, 0.f);
        }

        mPosition = focal + offset;
    }

    void Camera::setMode(Mode newMode, bool force)
    {
        if (mMode == newMode)
            return;
        Mode oldMode = mMode;
        if (!force && (newMode == Mode::FirstPerson || oldMode == Mode::FirstPerson) && !(mAnimation && mAnimation->busy))
        {
            // Changing the view will stop all playing animations, so if we are playing
            // anything important, queue the view change for later
            mQueuedMode = newMode;
            return;
        }
        mMode = newMode;
        mQueuedMode = std::nullopt;
        if (newMode == Mode::FirstPerson)
            mFirstPersonView = true;
        else if (newMode == Mode::ThirdPerson)
            mFirstPersonView = false;
        calculateDeferredRotation();
        if (oldMode == Mode::FirstPerson || newMode == Mode::FirstPerson)
        {
            instantTransition();
            mProcessViewChange = true;
        }
    }

    void Camera::setFocalPointTargetOffset(const osg::Vec2d& v)
    {
        mFocalPointTargetOffset = v;
        mPreviousTransitionSpeed = mFocalPointTransitionSpeed;
        mPreviousTransitionInfluence = 1.0f;
    }

    void Camera::updateFocalPointOffset(float duration)
    {
        if (duration <= 0)
            return;

        if (mSkipFocalPointTransition)
        {
            mSkipFocalPointTransition = false;
            mPreviousExtraOffset = osg::Vec2d();
            mPreviousTransitionInfluence = 0.f;
            mFocalPointCurrentOffset = mFocalPointTargetOffset;
        }

        osg::Vec2d oldOffset = mFocalPointCurrentOffset;

        if (mPreviousTransitionInfluence > 0)
        {
            mFocalPointCurrentOffset -= mPreviousExtraOffset;
            mPreviousExtraOffset = mPreviousExtraOffset / mPreviousTransitionInfluence + mPreviousTransitionSpeed * duration;
            mPreviousTransitionInfluence =
                std::max(0.f, mPreviousTransitionInfluence - duration * mFocalPointTransitionSpeedCoef);
            mPreviousExtraOffset *= mPreviousTransitionInfluence;
            mFocalPointCurrentOffset += mPreviousExtraOffset;
        }

        osg::Vec2d delta = mFocalPointTargetOffset - mFocalPointCurrentOffset;
        if (delta.length2() > 0)
        {
            float coef = duration * (1.0 + 5.0 / delta.length()) *
                         mFocalPointTransitionSpeedCoef * (1.0f - mPreviousTransitionInfluence);
            mFocalPointCurrentOffset += delta * std::min(coef, 1.0f);
        }
        else
        {
            mPreviousExtraOffset = osg::Vec2d();
            mPreviousTransitionInfluence = 0.f;
        }

        mFocalPointTransitionSpeed = (mFocalPointCurrentOffset - oldOffset) / duration;
    }

    void Camera::toggleViewMode(bool force)
    {
        setMode(mFirstPersonView ? Mode::ThirdPerson : Mode::FirstPerson, force);
    }

    bool Camera::toggleVanityMode(bool enable)
    {
        if (!enable)
            setMode(mFirstPersonView ? Mode::FirstPerson : Mode::ThirdPerson, false);
        else if (mVanityAllowed)
            setMode(Mode::Vanity, false);
        return (mMode == Mode::Vanity) == enable;
    }

    void Camera::setSneakOffset(float offset)
    {
        if (mAnimation)
            mAnimation->firstPersonOffset = {0,0,-offset};
    }

    void Camera::setYaw(float angle, bool force)
    {
        if (!mLockYaw || force)
            mYaw = Misc::normalizeAngle(angle);
        if (force)
            mLockYaw = true;
    }

    void Camera::setPitch(float angle, bool force)
    {
        const float epsilon = 0.000001f;
        float limit = static_cast<float>(osg::PI_2) - epsilon;
        if (!mLockPitch || force)
            mPitch = std::clamp(angle, -limit, limit);
        if (force)
            mLockPitch = true;
    }

    void Camera::setStaticPosition(const osg::Vec3d& pos)
    {
        if (mMode != Mode::Static)
            throw std::runtime_error("setStaticPosition can be used only if camera is in Static mode");
        mPosition = pos;
    }

    void Camera::setAnimation(Player *anim)
    {
        mAnimation = anim;
        mProcessViewChange = true;
    }

    void Camera::processViewChange()
    {
        mTrackingPath = {};
        if (mTrackingPtr.isEmpty() || !mAnimation)
            return;
        if (mMode == Mode::FirstPerson)
        {
            mAnimation->setViewMode(Npc::ViewMode::FirstPerson);
            auto path = mAnimation->worldTransform("Camera");
            if (path.empty())
                path = mAnimation->worldTransform("Head");
            for (auto &n : path)
                mTrackingPath.emplace_back(n);
            mHeightScale = 1.f;
        }
        else
        {
            mAnimation->setViewMode(Npc::ViewMode::Normal);
            mHeightScale = mAnimation->transform()->scale.z;
        }
        if (mTrackingPath.empty())
            mTrackingPath = {vsg::ref_ptr{mAnimation->transform()}};
        mProcessViewChange = false;
    }

    void Camera::applyDeferredPreviewRotationToPlayer(float dt)
    {
        if (mMode != Mode::ThirdPerson || mTrackingPtr.isEmpty())
            return;

        osg::Vec3f rot = mDeferredRotation;
        float delta = rot.normalize();
        delta = std::min(delta, (delta + 1.f) * 3 * dt);
        rot *= delta;
        mDeferredRotation -= rot;

        if (mDeferredRotationDisabled)
        {
            mDeferredRotationDisabled = delta > 0.0001;
            rotateCameraToTrackingPtr();
            return;
        }

        auto& movement = mTrackingPtr.getClass().getMovementSettings(mTrackingPtr);
        movement.mRotation[0] += rot.x();
        movement.mRotation[1] += rot.y();
        movement.mRotation[2] += rot.z();
        if (std::abs(mDeferredRotation.z()) > 0.0001)
        {
            float s = std::sin(mDeferredRotation.z());
            float c = std::cos(mDeferredRotation.z());
            float x = movement.mPosition[0];
            float y = movement.mPosition[1];
            movement.mPosition[0] = x *  c + y * s;
            movement.mPosition[1] = x * -s + y * c;
        }
    }

    void Camera::rotateCameraToTrackingPtr()
    {
        if (mMode == Mode::Static || mTrackingPtr.isEmpty())
            return;
        setPitch(-mTrackingPtr.getRefData().getPosition().rot[0] - mDeferredRotation.x());
        setYaw(-mTrackingPtr.getRefData().getPosition().rot[2] - mDeferredRotation.z());
    }

    void Camera::instantTransition()
    {
        mSkipFocalPointTransition = true;
        mDeferredRotationDisabled = false;
        mDeferredRotation = osg::Vec3f();
        rotateCameraToTrackingPtr();
    }

    void Camera::calculateDeferredRotation()
    {
        if (mMode == Mode::Static)
        {
            mDeferredRotation = osg::Vec3f();
            return;
        }
        MWWorld::Ptr ptr = mTrackingPtr;
        if (mMode == Mode::Preview || mMode == Mode::Vanity || ptr.isEmpty())
            return;
        if (mFirstPersonView)
        {
            instantTransition();
            return;
        }

        mDeferredRotation.x() = Misc::normalizeAngle(-ptr.getRefData().getPosition().rot[0] - mPitch);
        mDeferredRotation.z() = Misc::normalizeAngle(-ptr.getRefData().getPosition().rot[2] - mYaw);
        if (!mDeferredRotationAllowed)
            mDeferredRotationDisabled = true;
    }

}
