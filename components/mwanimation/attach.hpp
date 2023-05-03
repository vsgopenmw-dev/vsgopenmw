#ifndef VSGOPENMW_MWANIMATION_ATTACH_H
#define VSGOPENMW_MWANIMATION_ATTACH_H

#include <string>
#include <vector>

#include <vsg/nodes/Group.h>

namespace Anim
{
    class Bones;
}
namespace MWAnim
{
    /*
     * Attaches required bones to skeleton and node to bone.
     */
    std::vector<vsg::Node*> attachBonesAndNode(vsg::ref_ptr<vsg::Node> node, vsg::Group& skeleton,
        Anim::Bones& bones, const std::string& bonename);

    /*
     * Attaches node to preattached bones.
     */
    std::vector<vsg::Node*> attachNode(vsg::ref_ptr<vsg::Node> node,
        Anim::Bones& bones, const std::string& bonename);
}

#endif
