#ifndef VSGOPENMW_MWANIM_EFFECT_H
#define VSGOPENMW_MWANIM_EFFECT_H

#include <vsg/nodes/Group.h>

#include <components/animation/autoplay.hpp>
#include <components/animation/animation.hpp>
#include <components/vsgutil/compilecontext.hpp>

#include "context.hpp"

namespace Anim
{
    class Transform;
}
namespace MWAnim
{
    class Object;

    struct Effect
    {
        vsg::ref_ptr<vsg::Node> node;
        MWAnim::Context mwctx;
        int effectId = -1;
        bool loop = false;

        Anim::AutoPlay update;
        float duration = 0;

        vsg::Group* parentBone{};
        MWAnim::Object* parentObject{};

        static std::pair<Anim::Animation, vsg::ref_ptr<vsg::Node>> load(const Context& mwctx, vsg::ref_ptr<vsg::Node> prototype, const std::string& overrideTexture, bool overrideAllTextures, const std::vector<vsg::ref_ptr<vsg::Node>>& replaceDummyNodes = {});

        void compile(Anim::Animation& animation, vsg::ref_ptr<vsg::Node> node, const std::vector<Anim::Transform*>& worldAttachmentPath, const std::vector<vsg::Node*>& localAttachmentPath);
        void attachTo(vsg::Group* bone);
        void attachTo(MWAnim::Object* obj);
        bool run(float dt);
        void detach();
    };
}

#endif
