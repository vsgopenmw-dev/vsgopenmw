#ifndef VSGOPENMW_MWRENDER_WATER_H
#define VSGOPENMW_MWRENDER_WATER_H

#include <components/vsgutil/composite.hpp>

namespace Resource
{
    class ResourceSystem;
}
namespace Anim
{
    class Transform;
}
namespace MWRender
{
    class Water : public vsgUtil::Composite<vsg::Node>
    {
        vsg::ref_ptr<Anim::Transform> mTransform;

    public:
        Water(Resource::ResourceSystem* resourceSystem);
        ~Water();

        void setHeight(float height);
        bool isUnderwater(const vsg::vec3& pos) const;

        /*
        /// adds an emitter, position will be tracked automatically using its scene node
        void addEmitter (const MWWorld::Ptr& ptr, float scale = 1.f, float force = 1.f);
        void removeEmitter (const MWWorld::Ptr& ptr);
        void updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr);
        void emitRipple(const osg::Vec3f& pos);
        void removeCell(const MWWorld::CellStore* store); ///< remove all emitters in this cell
        void clearRipples();
        */
    };
}

#endif
