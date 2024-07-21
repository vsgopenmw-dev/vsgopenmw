#ifndef VSGOPENMW_MWANIM_EFFECTS_H
#define VSGOPENMW_MWANIM_EFFECTS_H

#include <optional>
#include <list>

#include <vsg/nodes/Group.h>

#include <components/vsgutil/attachable.hpp>

namespace Anim
{
    class Transform;
}
namespace MWAnim
{
    class Object;
    class Effect;

    /*
     * Meta object that can be added to a node/scene graph's auxiliary container for keeping track of active effects attached to that scene graph.
     * During update(dt), any expired effects will be removed automatically.
     */
    class Effects : public vsgUtil::Attachable<Effects, vsg::Node>
    {
    public:
        std::list<Effect> effects;
        static const std::string sAttachKey;

        void remove_if(std::function<bool(Effect&)> f);
        void remove(std::optional<int> effectId = {});
        void update(float dt);
    };

    /**
     * @brief Add an effect mesh
     * @param effectId An ID for this effect by which you can identify it later. If this is not wanted, set to -1.
     * @param loop Loop the effect. If false, it is removed automatically after it finishes playing. If true,
     *              you need to remove it manually using removeEffect when the effect should end.
     * @param texture override the texture specified in the model's materials - if empty, do not override
     */
    void addEffect(MWAnim::Object& obj, vsg::Group* attachBone, vsg::ref_ptr<vsg::Node> node, const std::vector<Anim::Transform*>& worldAttachmentPath,
        int effectId, bool loop = false, const std::string& overrideTexture = {}, bool overrideAllTextures = false);
    void updateEffects(vsg::Node& root, float dt);
    void removeEffect(vsg::Node& root, std::optional<int> effectId);
    void getLoopingEffects(const vsg::Node& root, std::vector<int>& out);
}

#endif
