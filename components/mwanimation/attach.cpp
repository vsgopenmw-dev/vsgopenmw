#include "attach.hpp"

#include <cassert>

#include <components/animation/attachbone.hpp>
#include <components/animation/bones.hpp>
#include <components/vsgutil/nodepath.hpp>

namespace MWAnim
{
    std::vector<vsg::Node*> attachBonesAndNode(vsg::ref_ptr<vsg::Node> node, vsg::Group& skeleton,
        Anim::Bones& bones, const std::string& bonename)
    {
        assert(!bones.attached);
        auto bone = bones.search(bonename);
        if (!bone)
            return {};
        /*
         * Attaching skins to the root node speeds up the record traversal, but slows down the skinning implementation and produces less accurate bounding spheres. Visually, there's no difference because of the skins' manual reference frame.
         */
        /*
        if (contents.contains(Anim::Contents::Skins))
            skeleton.addChild(node);
        else
        */
        {
            // openmw-5280-skin-attachment-node
            auto attachGroup = Anim::getOrAttachBone(&skeleton, bone->path);
            // vsgUtil::addChildren(*attachGroup, *node);
            attachGroup->addChild(node);
            auto path = vsgUtil::path<vsg::Node*>(bone->path);
            path.emplace_back(node);
            return path;
        }
    }

    std::vector<vsg::Node*> attachNode(vsg::ref_ptr<vsg::Node> node,
        Anim::Bones& bones, const std::string& bonename)
    {
        assert(bones.attached);
        auto bone = bones.search(bonename);
        if (!bone)
            return {};
        bone->path.back()->subgraphRequiresLocalFrustum = true;
        bone->path.back()->addChild(node);
        auto path = vsgUtil::path<vsg::Node*>(bone->path);
        path.emplace_back(node);
        return path;
    }
}
