#ifndef VSGOPENMW_MWSTATE_GAMESTATE_H
#define VSGOPENMW_MWSTATE_GAMESTATE_H

namespace MWState
{
    /*
     * Provides frame update entrypoint.
     */
    class GameState
    {
    public:
        virtual ~GameState() {};
        virtual bool /*keepRunning*/ run(float dt) = 0;

        bool showCursor = true;
        bool disableControls = false;
        bool disableEvents = false;
        bool requiresCompile = false;
        bool requiresScene = true;
    };
}

#endif
