#ifndef VSGOPENMW_MWSTATE_MENU_H
#define VSGOPENMW_MWSTATE_MENU_H

#include "gui.hpp"

namespace MWState
{
    /*
     * Pauses game.
     */
    class Menu : public Gui
    {
    public:
        Menu();
        bool run(float dt) override;
    };
}

#endif
