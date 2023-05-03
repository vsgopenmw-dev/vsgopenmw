#include "static.hpp"

#include <components/esm3/loadstat.hpp>
//#include <components/esm4/loadstat.hpp>

#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/vismask.hpp"

#include "classmodel.hpp"

namespace MWClass
{
    Static::Static()
        : MWWorld::RegisteredClass<Static>(ESM::Static::sRecordId)
    {
    }

    void Static::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Static::insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        insertObjectPhysics(ptr, model, rotation, physics);
    }

    void Static::insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        physics.addObject(ptr, model, rotation, MWPhysics::CollisionType_World);
    }

    std::string Static::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Static>(ptr);
    }

    std::string_view Static::getName(const MWWorld::ConstPtr& ptr) const
    {
        return {};
    }

    bool Static::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return false;
    }

    MWWorld::Ptr Static::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Static>* ref = ptr.get<ESM::Static>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    /*
    ESM4Static::ESM4Static()
        : MWWorld::RegisteredClass<ESM4Static>(ESM4::Static::sRecordId)
    {
    }

    void ESM4Static ::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void ESM4Static::insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        insertObjectPhysics(ptr, model, rotation, physics);
    }

    void ESM4Static::insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        physics.addObject(ptr, model, rotation, MWPhysics::CollisionType_World);
    }

    std::string ESM4Static::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM4::Static>(ptr);
    }

    std::string_view ESM4Static ::getName(const MWWorld::ConstPtr& ptr) const
    {
        return {};
    }

    bool ESM4Static::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return false;
    }

    MWWorld::Ptr ESM4Static::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM4::Static>* ref = ptr.get<ESM4::Static>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }
    */
}
