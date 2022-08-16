#ifndef OPENMW_MWPHYSICS_HEIGHTFIELD_H
#define OPENMW_MWPHYSICS_HEIGHTFIELD_H

#include <vsg/core/ref_ptr.h>

#include <LinearMath/btScalar.h>

#include <memory>
#include <vector>

class btCollisionObject;
class btHeightfieldTerrainShape;

namespace vsg
{
    class Object;
}

namespace MWPhysics
{
    class PhysicsTaskScheduler;

    class HeightField
    {
    public:
        HeightField(const float* heights, int x, int y, int size, int verts, float minH, float maxH,
                    const vsg::Object* holdObject, PhysicsTaskScheduler* scheduler);
        ~HeightField();

        btCollisionObject* getCollisionObject();
        const btCollisionObject* getCollisionObject() const;
        const btHeightfieldTerrainShape* getShape() const;

    private:
        std::unique_ptr<btHeightfieldTerrainShape> mShape;
        std::unique_ptr<btCollisionObject> mCollisionObject;
        vsg::ref_ptr<const vsg::Object> mHoldObject;
#if BT_BULLET_VERSION < 310
        std::vector<btScalar> mHeights;
#endif

        PhysicsTaskScheduler* mTaskScheduler;

        void operator=(const HeightField&);
        HeightField(const HeightField&);
    };
}

#endif
