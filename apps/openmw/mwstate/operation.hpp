#ifndef VSGOPENMW_MWSTATE_OPERATION_H
#define VSGOPENMW_MWSTATE_OPERATION_H

#include "gamestate.hpp"

namespace MWState
{
    /*
     * Manipulates game once.
     */
    struct Operation : public GameState
    {
        Operation()
        {
            disableEvents = true;
            disableControls = true;
        }
    };
}

#endif
