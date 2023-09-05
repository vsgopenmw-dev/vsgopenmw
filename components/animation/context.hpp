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

    /*
     * Optionally provides scene graph context to animation updates.
     */
    struct Context
    {
        // Controls skin reference frame.
        std::vector<vsg::Node*> attachmentPath;
        // Supports skins.
        Bones* bones{};
        // Toggles nodes.
        const Mask* mask{};
    };
}

#endif
