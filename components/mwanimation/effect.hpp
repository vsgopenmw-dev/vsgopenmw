#ifndef VSGOPENMW_MWANIM_EFFECT_H
#define VSGOPENMW_MWANIM_EFFECT_H

#include <vsg/nodes/Group.h>

#include <components/animation/update.hpp>

namespace MWAnim
{
    class Context;

    struct Effect
    {
        vsg::ref_ptr<vsg::Node> node;
        int effectId = -1;
        bool loop = false;

        std::string overrideTexture;
        bool overrideAllTextures;
        std::vector<vsg::ref_ptr<vsg::Node>> replaceDummyNodes;

        Anim::AutoUpdate update;
        float duration = 0;

        vsg::Group *parent{};

        void compile(const MWAnim::Context &mwctx);
        void attachTo(vsg::Group *p);
        bool run(float dt);
        void detach();
   };
}

#endif
