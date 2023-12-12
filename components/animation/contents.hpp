#ifndef VSGOPENMW_ANIMATION_CONTENTS_H
#define VSGOPENMW_ANIMATION_CONTENTS_H

namespace Anim
{
    /*
     * Contents is a collection of flags indicating the type of animations contained in a model.
     * Typically attached as a meta object to the root of the model, and used to decide what kind of optimizations like cull nodes, instancing etc. are compatible with it.
     */
    struct Contents
    {
        enum
        {
            // Controller is the base class for all types of animations (except Placeholders).
            Controllers = 1,
            // If we have any transform controllers then the position/rotation/scales of transforms will be animated and can't be optimized away.
            TransformControllers = (1 << 1),
            // If we have any particle effects, then the vsg::View needs to be set up with a bin for compute shaders and the animation context should specify a particle mask.
            Particles = (1 << 2),
            // If skeletal animation is used, the animation context needs to be set up with a bones object for the bone hierarchy.
            Skins = (1 << 3),
            // Placeholders are empty dummy nodes to be replaced by the user if required.
            // Used to specify attachment points for an optional light source or other effects added in specific conditions.
            Placeholders = (1 << 4),
            // Indicates that Cull/DepthSorted node bound values will be changed by animations.
            DynamicBounds = (1 << 5)
            //Switches =
        };
        int contents{};
        bool empty() const { return contents == 0; }
        void add(int c) { contents |= c; }
        bool contains(int c) const { return contents & c; }
    };
}

#endif
