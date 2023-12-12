#ifndef VSGOPENMW_MWRENDER_EFFECTS_H
#define VSGOPENMW_MWRENDER_EFFECTS_H

#include <components/vsgutil/composite.hpp>
#include <components/mwanimation/context.hpp>

namespace MWAnim
{
    class Effects;
}
namespace MWRender
{
    // Note: effects attached to another object should be managed by MWRender::addEffect.
    // This class manages "free" effects, i.e. attached to a dedicated scene node in the world.
    class Effects : public vsgUtil::Composite<vsg::Group>
    {
    public:
        Effects(const MWAnim::Context& ctx);
        ~Effects();

        /// Add an effect. When it's finished playing, it will be removed automatically.
        void add(const std::string& model, const std::string& textureOverride, const vsg::vec3& worldPosition,
            float scale, bool isMagicVFX = true);

        void update(float dt);

        /// Remove all effects
        void clear();

    private:
        vsg::ref_ptr<MWAnim::Effects> mEffects;
        MWAnim::Context mContext;
    };
}

#endif
