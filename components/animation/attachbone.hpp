#ifndef VSGOPENMW_ANIMATION_ATTACHBONE_H
#define VSGOPENMW_ANIMATION_ATTACHBONE_H

#include "bones.hpp"
#include "transform.hpp"

namespace Anim
{
    /*
     * Attaches required bones.
     */
    inline vsg::ref_ptr<vsg::Group> getOrAttachBone(vsg::Group *attachTo, const BonePath &path)
    {
        for (auto &node : path)
        {
            auto it = std::find(attachTo->children.begin(), attachTo->children.end(), node);
            if (it == attachTo->children.end())
                attachTo->addChild(vsg::ref_ptr{node});
            if (/*wantToAttachCullNodes &&*/node == path.back())
                node->subgraphRequiresLocalFrustum = true;
            attachTo = node;
        }
        return vsg::ref_ptr{attachTo};
    }
}

#endif
