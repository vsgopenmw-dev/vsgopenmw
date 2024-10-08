#ifndef OPENMW_MWWORLD_PROJECTILEMANAGER_H
#define OPENMW_MWWORLD_PROJECTILEMANAGER_H

#include <optional>
#include <set>
#include <string>

#include <osg/Vec4>
#include <vsg/maths/quat.h>
#include <vsg/maths/vec4.h>

#include <components/esm3/effectlist.hpp>

#include "ptr.hpp"

namespace MWPhysics
{
    class PhysicsSystem;
}

namespace Loading
{
    class Listener;
}

namespace osg
{
    class Quat;
}

namespace MWRender
{
    class RenderManager;
    class ProjectileHandle;
}
namespace MWSound
{
    class Sound;
}

namespace MWWorld
{

    class ProjectileManager
    {
    public:
        ProjectileManager(MWRender::RenderManager* rendering, MWPhysics::PhysicsSystem* physics);

        /// If caster is an actor, the actor's facing orientation is used. Otherwise fallbackDirection is used.
        void launchMagicBolt(const ESM::RefId& spellId, const MWWorld::Ptr& caster, const osg::Vec3f& fallbackDirection,
            ESM::RefNum item);

        void launchProjectile(const MWWorld::Ptr& actor, const MWWorld::ConstPtr& projectile, const osg::Vec3f& pos,
            const osg::Quat& orient, const MWWorld::Ptr& bow, float speed, float attackStrength);

        void updateCasters();

        void update(float dt);

        void processHits();

        /// Removes all current projectiles. Should be called when switching to a new worldspace.
        void clear();

        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;
        bool readRecord(ESM::ESMReader& reader, uint32_t type);
        int countSavedGameRecords() const;

    private:
        MWRender::RenderManager* mRender;
        MWPhysics::PhysicsSystem* mPhysics;
        float mCleanupTimer;

        struct State
        {
            std::shared_ptr<MWRender::ProjectileHandle> handle;
            int mActorId;
            int mProjectileId;

            // TODO: this will break when the game is saved and reloaded, since there is currently
            // no way to write identifiers for non-actors to a savegame.
            MWWorld::Ptr mCasterHandle;

            MWWorld::Ptr getCaster();

            // MW-ids of a magic projectile
            std::vector<ESM::RefId> mIdMagic;

            // MW-id of an arrow projectile
            ESM::RefId mIdArrow;

            bool mToDelete;
        };

        struct MagicBoltState : public State
        {
            ESM::RefId mSpellId;

            // Name of item to display as effect source in magic menu (in case we casted an enchantment)
            std::string mSourceName;

            ESM::EffectList mEffects;

            float mSpeed;
            // Refnum of the casting item
            ESM::RefNum mItem;

            std::vector<MWSound::Sound*> mSounds;
            std::set<ESM::RefId> mSoundIds;
        };

        struct ProjectileState : public State
        {
            // RefID of the bow or crossbow the actor was using when this projectile was fired (may be empty)
            ESM::RefId mBowId;

            osg::Vec3f mVelocity;
            float mAttackStrength;
            bool mThrown;
        };

        std::vector<MagicBoltState> mMagicBolts;
        std::vector<ProjectileState> mProjectiles;

        void cleanupProjectile(ProjectileState& state);
        void cleanupMagicBolt(MagicBoltState& state);
        void periodicCleanup(float dt);

        void moveProjectiles(float dt);
        void moveMagicBolts(float dt);

        void createModel(State& state, const std::string& model, const vsg::vec3& pos, const vsg::quat& orient,
            bool rotate, std::optional<vsg::vec3> lightDiffuseColor = {}, std::optional<vsg::vec4> glowColor = {},
            std::string texture = "");

        void operator=(const ProjectileManager&);
        ProjectileManager(const ProjectileManager&);
    };

}

#endif
