#ifndef VSGOPENMW_ANIMATION_CONTEXT_H
#define VSGOPENMW_ANIMATION_CONTEXT_H

namespace vsg
{
    class Node;
}
namespace Anim
{
    class Bones;
    class Mask;
    class Transform;

    /*
     * Optionally provides scene graph context to animation updates.
     */
    struct Context
    {
        // worldAttachmentPath is the path of transforms leading from the root of the view/scene to the animation's reference frame/skeleton root.
        // Useful for animation features that need to use a world reference frame, like a trail of particles.
        std::vector<Transform*> worldAttachmentPath;
        // attachmentPath is the remaining path leading from the skeleton root to the node that is being linked/attached. If there are no bones, the path should contain the node to attach as its only element.
        // Note, concatenating the worldAttachmentPath and the attachmentPath gives the complete path from the root of the view/scene to the node that is being linked/attached.
        std::vector<vsg::Node*> attachmentPath;
        // Bones object used by the skinning implementation to get the current transformation of bones.
        Bones* bones{};
        // Optional mask object for turning off undesired features.
        const Mask* mask{};
    };
}

#endif
