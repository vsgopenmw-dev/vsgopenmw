#ifndef VSGOPENMW_ANIMATION_CONTEXT_H
#define VSGOPENMW_ANIMATION_CONTEXT_H

#include <vector>

#include <vsg/core/ref_ptr.h>

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
        // Detects attachment path.
        vsg::Node *scene{};
        // Supports skins.
        Bones *bones{};
        // Toggles nodes.
        const Mask *mask{};
    };
}

#endif
