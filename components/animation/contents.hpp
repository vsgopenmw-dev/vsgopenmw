#ifndef VSGOPENMW_ANIMATION_CONTENTS_H
#define VSGOPENMW_ANIMATION_CONTENTS_H

namespace Anim
{
    /*
     * Signifies contained animations.
     */
    struct Contents
    {
        enum
        {
            Controllers = 1,
            TransformControllers = (1<<1),
            Particles = (1<<2),
            Skins = (1<<3),
            Placeholders = (1<<4)
            //Switches
        };
        int contents{};
        bool empty() const { return contents == 0; }
        void add(int c)
        {
            contents |= c;
        }
        bool contains(int c) const
        {
            return contents & c;
        }
    };
}

#endif
