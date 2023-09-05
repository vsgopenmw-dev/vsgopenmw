#ifndef VSGOPENMW_MWRENDER_SCENE_H
#define VSGOPENMW_MWRENDER_SCENE_H

#include <map>

#include <vsg/core/ref_ptr.h>
#include <vsg/nodes/Group.h>

#include <components/mwanimation/context.hpp>

#include "renderinginterface.hpp"

namespace MWWorld
{
    class CellStore;
}
namespace MWAnim
{
    class Object;
}
namespace vsgUtil
{
    class UpdateThreads;
}
namespace MWRender
{
    /*
     * Manages attached objects.
     */
    class Scene : public RenderingInterface
    {
        void operator=(const Scene&);
        Scene(const Scene&);
        vsg::ref_ptr<vsgUtil::UpdateThreads> mUpdateThreads;
        using Objects = std::map<MWWorld::Ptr, std::unique_ptr<MWAnim::Object>>;
        struct Cell;
        using CellMap = std::map<const MWWorld::CellStore*, Cell>;
        CellMap mObjectCells;
        CellMap mActorCells;

        vsg::ref_ptr<vsg::Switch> mActorGroup;
        vsg::ref_ptr<vsg::Switch> mObjectGroup;
        Cell& getOrCreateCell(const MWWorld::Ptr& ptr, bool actor);
        Cell* getCell(const MWWorld::Ptr& ptr, bool actor);

    public:
        Scene(size_t updateThreads);
        ~Scene();

        MWAnim::Context context;
        MWAnim::Context noParticlesContext;
        vsg::ref_ptr<vsg::Node> getObjects();
        vsg::ref_ptr<vsg::Node> getActors();

        /// @param animated Attempt to load separate keyframes from a .kf file matching the model file?
        /// @param allowLight If false, no lights will be created, and particles systems will be removed.
        void insertModel(
            const MWWorld::Ptr& ptr, const std::string& model, bool allowLight = true) override;

        void insertNPC(const MWWorld::Ptr& ptr) override;
        void insertCreature(const MWWorld::Ptr& ptr, const std::string& model, bool weaponsShields) override;

        MWAnim::Object* getObject(const MWWorld::Ptr& ptr);

        bool removeObject(const MWWorld::Ptr& ptr);
        ///< \return found?

        void removeCell(const MWWorld::CellStore* store);

        void updatePtr(const MWWorld::Ptr& old, const MWWorld::Ptr& cur);

        MWWorld::Ptr getPtr(const vsg::Node* node);
        MWWorld::Ptr getPtr(const vsg::Node* node, CellMap& map);
        void update(float dt);
        vsg::ref_ptr<vsgUtil::UpdateThreads> getUpdateThreads();
    };

}
#endif
