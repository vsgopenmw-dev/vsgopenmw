#include "object.hpp"
#include "mtphysics.hpp"

#include <vsg/maths/transform.h>
#include <vsg/nodes/Transform.h>

#include <components/animation/transform.hpp>
#include <components/bullethelpers/collisionobject.hpp>
#include <components/debug/debuglog.hpp>
#include <components/misc/convert.hpp>
#include <components/mwanimation/object.hpp>
#include <components/resource/bulletshape.hpp>
#include <components/vsgutil/id.hpp>
#include <components/vsgutil/traverse.hpp>
#include <components/vsgutil/nodepath.hpp>

#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <LinearMath/btTransform.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace
{
    class GetNodePath : public vsgUtil::SearchPath<const vsg::Transform*>, public vsgUtil::TConstTraverse<vsg::Node>
    {
    public:
        int needle{};
        using vsg::ConstVisitor::apply;
        void apply(const vsg::Transform& node) override
        {
            auto ppn = pushPop(&node);
            auto id = vsgUtil::ID::get(node);
            if (id && id->id == needle)
                foundPath = this->path;
            else
                traverseNode(node, *this);
        }
    };
}
namespace MWPhysics
{
    Object::Object(const MWWorld::Ptr& ptr, vsg::ref_ptr<Resource::BulletShapeInstance> shapeInstance,
        osg::Quat rotation, int collisionType, PhysicsTaskScheduler* scheduler)
        : PtrHolder(ptr, osg::Vec3f())
        , mShapeInstance(std::move(shapeInstance))
        , mSolid(true)
        , mScale(ptr.getCellRef().getScale(), ptr.getCellRef().getScale(), ptr.getCellRef().getScale())
        , mPosition(ptr.getRefData().getPosition().asVec3())
        , mRotation(rotation)
        , mTaskScheduler(scheduler)
    {
        mCollisionObject = BulletHelpers::makeCollisionObject(mShapeInstance->mCollisionShape.get(),
            Misc::Convert::toBullet(mPosition), Misc::Convert::toBullet(rotation));
        mCollisionObject->setUserPointer(this);
        mShapeInstance->setLocalScaling(mScale);
        mTaskScheduler->addCollisionObject(mCollisionObject.get(), collisionType,
            CollisionType_Actor | CollisionType_HeightMap | CollisionType_Projectile);
    }

    Object::~Object()
    {
        mTaskScheduler->removeCollisionObject(mCollisionObject.get());
    }

    const vsg::ref_ptr<Resource::BulletShapeInstance> Object::getShapeInstance() const
    {
        return mShapeInstance;
    }

    void Object::setScale(float scale)
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        mScale = { scale, scale, scale };
        mScaleUpdatePending = true;
    }

    void Object::setRotation(osg::Quat quat)
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        mRotation = quat;
        mTransformUpdatePending = true;
    }

    void Object::updatePosition()
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        mPosition = mPtr.getRefData().getPosition().asVec3();
        mTransformUpdatePending = true;
    }

    void Object::commitPositionChange()
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        if (mScaleUpdatePending)
        {
            mShapeInstance->setLocalScaling(mScale);
            mScaleUpdatePending = false;
        }
        if (mTransformUpdatePending)
        {
            btTransform trans;
            trans.setOrigin(Misc::Convert::toBullet(mPosition));
            trans.setRotation(Misc::Convert::toBullet(mRotation));
            mCollisionObject->setWorldTransform(trans);
            mTransformUpdatePending = false;
        }
    }

    btTransform Object::getTransform() const
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        btTransform trans;
        trans.setOrigin(Misc::Convert::toBullet(mPosition));
        trans.setRotation(Misc::Convert::toBullet(mRotation));
        return trans;
    }

    bool Object::isSolid() const
    {
        return mSolid;
    }

    void Object::setSolid(bool solid)
    {
        mSolid = solid;
    }

    bool Object::isAnimated() const
    {
        return mShapeInstance->isAnimated();
    }

    bool Object::animateCollisionShapes()
    {
        if (mShapeInstance->mAnimatedShapes.empty())
            return false;

        assert(mShapeInstance->mCollisionShape->isCompound());

        btCompoundShape* compound = static_cast<btCompoundShape*>(mShapeInstance->mCollisionShape.get());
        bool result = false;
        for (const auto& [recIndex, shapeIndex] : mShapeInstance->mAnimatedShapes)
        {
            auto nodePathFound = mRecIndexToNodePath.find(recIndex);
            if (nodePathFound == mRecIndexToNodePath.end())
            {
                if (auto anim = MWBase::Environment::get().getWorld()->getAnimation(mPtr))
                {
                    GetNodePath visitor;
                    visitor.needle = recIndex;
                    anim->transform()->traverse(visitor);

                    if (visitor.foundPath.empty())
                    {
                        // Remove nonexistent transforms from animated shapes map and early out
                        mShapeInstance->mAnimatedShapes.erase(recIndex);
                        return false;
                    }
                    nodePathFound = mRecIndexToNodePath.emplace(recIndex, visitor.foundPath).first;
                }
            }

            auto& nodePath = nodePathFound->second;
            auto matrix = vsg::mat4(vsg::computeTransform(nodePath));
            // matrix.orthoNormalize(matrix);

            btTransform transform;
            auto pos = matrix[3];
            transform.setOrigin(btVector3(pos.x, pos.y, pos.z) * compound->getLocalScaling());
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    transform.getBasis()[i][j] = matrix(j, i); // NB column/row major difference

            // Note: we can not apply scaling here for now since we treat scaled shapes
            // as new shapes (btScaledBvhTriangleMeshShape) with 1.0 scale for now
            if (!(transform == compound->getChildTransform(shapeIndex)))
            {
                compound->updateChildTransform(shapeIndex, transform);
                result = true;
            }
        }
        return result;
    }
}
