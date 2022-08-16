#ifndef VSGOPENMW_MWANIMATION_ATTACH_H
#define VSGOPENMW_MWANIMATION_ATTACH_H

#include <string>
#include <vector>

#include <vsg/nodes/Group.h>

#include <components/animation/contents.hpp>

namespace Anim
{
    class Bones;
}
namespace MWAnim
{
    /*
     * Attaches animation part to skeleton.
     */
    std::vector<vsg::Node*> attach(vsg::ref_ptr<vsg::Node> node, Anim::Contents contents, vsg::Group &skeleton, Anim::Bones &bones, const std::string &bonename);
}

#endif
