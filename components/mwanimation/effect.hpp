#ifndef VSGOPENMW_MWANIM_EFFECT_H
#define VSGOPENMW_MWANIM_EFFECT_H

#include <vsg/nodes/Group.h>

#include <components/animation/autoplay.hpp>
#include <components/vsgutil/compilecontext.hpp>

#include "context.hpp"

namespace MWAnim
{
    struct Effect
    {
        vsg::ref_ptr<vsg::Node> node;
        MWAnim::Context mwctx;
        int effectId = -1;
        bool loop = false;

        std::string overrideTexture;
        bool overrideAllTextures;
        std::vector<vsg::ref_ptr<vsg::Node>> replaceDummyNodes;

        Anim::AutoPlay update;
        float duration = 0;

        vsg::Group* parent{};

        void compile();
        void attachTo(vsg::Group* p);
        bool run(float dt);
        void detach();
    };
}

#endif
