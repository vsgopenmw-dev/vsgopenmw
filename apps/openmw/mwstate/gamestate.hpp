#ifndef VSGOPENMW_MWSTATE_GAMESTATE_H
#define VSGOPENMW_MWSTATE_GAMESTATE_H

#include <optional>

namespace MWState
{
    /*
     * Provides frame update entrypoint.
     */
    class GameState
    {
    public:
        virtual ~GameState(){}
        virtual bool /*keepRunning*/ run(float dt) = 0;

        std::optional<bool> showCursor;
        bool disableControls = false;
        bool disableEvents = false;
        bool requiresScene = true;
    };
}

#endif
