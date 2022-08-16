#ifndef GAME_RENDERING_INTERFACE_H
#define GAME_RENDERING_INTERFACE_H

#include "scene.hpp"

namespace MWRender
{
    using Objects = Scene;

    class RenderingInterface
    {
    public:
        virtual Scene &getObjects() = 0;
        virtual ~RenderingInterface(){}
    };
}

#endif
