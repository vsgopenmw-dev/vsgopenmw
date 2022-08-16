#ifndef VSGOPENMW_MWRENDER_PROJECTILES_H
#define VSGOPENMW_MWRENDER_PROJECTILES_H

#include <vsg/nodes/Group.h>

#include <components/vsgutil/composite.hpp>

namespace MWAnim
{
    class Context;
}
namespace MWRender
{
    class Projectile;

    struct ProjectileHandle
    {
        vsg::vec3 position;
        vsg::quat attitude;
        bool remove = false;
    };

    class Projectiles : public vsgUtil::Composite<vsg::Group>
    {
        std::list<Projectile> mProjectiles;
        const MWAnim::Context &mContext;
        vsg::ref_ptr<vsg::Group> mEnv;
    public:
        Projectiles(const MWAnim::Context &ctx);
        ~Projectiles();

        std::shared_ptr<ProjectileHandle> add(const std::string &model, const vsg::vec3 &pos, const vsg::quat &orient, bool autoRotate, std::optional<vsg::vec3> lightColor, std::optional<vsg::vec4> glowColor, const std::string textureOverride, const std::vector<std::string> additionalModels);

        void update(float dt);
    };
}

#endif
