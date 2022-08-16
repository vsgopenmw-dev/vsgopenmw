#include "attach.hpp"

#include <components/animation/bones.hpp>
#include <components/animation/attachbone.hpp>
//#include <components/vsgutil/addchildren.hpp>

namespace MWAnim
{
    std::vector<vsg::Node*> attach(vsg::ref_ptr<vsg::Node> node, Anim::Contents contents, vsg::Group &skeleton, Anim::Bones &bones, const std::string &bonename)
    {
        auto bone = bones.search(bonename);
        if (!bone)
            return {};
        if (contents.contains(Anim::Contents::Skins))
        {
            //vsgUtil::addChildren(skeleton, *node);
            skeleton.addChild(node);
            return {&skeleton, node};
        }
        else
        {
            //assert(boneIsUnique)
            auto attachGroup = Anim::getOrAttachBone(&skeleton, bone->path);
            //vsgUtil::addChildren(*attachGroup, *node);
            attachGroup->addChild(node);
            std::vector<vsg::Node*> path;
            std::copy(bone->path.begin(), bone->path.end(), std::back_inserter(path));
            path.emplace_back(node);
            return path;
        }
    }
}
