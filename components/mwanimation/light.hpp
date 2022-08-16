#ifndef VSGOPENMW_MWANIMATION_LIGHT_H
#define VSGOPENMW_MWANIMATION_LIGHT_H

#include <vsg/nodes/Group.h>

#include <components/animation/controllers.hpp>

namespace ESM
{
    class Light;
}
namespace MWAnim
{
    vsg::Node *addLight(vsg::Group *attachLight, vsg::Group *fallback, const ESM::Light &light, Anim::Controllers &controllers);
}

#endif
