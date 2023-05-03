#ifndef GAME_RENDERING_INTERFACE_H
#define GAME_RENDERING_INTERFACE_H

#include <string>

#include "../mwworld/ptr.hpp"

namespace MWRender
{
    struct RenderingInterface
    {
        virtual void insertModel(
            const MWWorld::Ptr& ptr, const std::string& model, bool allowLight = true) = 0;
        virtual void insertNPC(const MWWorld::Ptr& ptr) = 0;
        virtual void insertCreature(const MWWorld::Ptr& ptr, const std::string& model, bool weaponsShields) = 0;
        RenderingInterface& getObjects() { return *this; } //vsgopenmw-delete-me
        virtual ~RenderingInterface() {}
    };
}

#endif
