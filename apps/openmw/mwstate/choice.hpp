#ifndef VSGOPENMW_MWSTATE_CHOICE_H
#define VSGOPENMW_MWSTATE_CHOICE_H

#include <string>
#include <vector>

#include "gamestate.hpp"

namespace MWState
{
    /*
     * Pauses game until choice is made.
     */
    class Choice : public GameState
    {
        bool mPushedDialog{};
    public:
        std::string message;
        std::vector<std::string> buttons;
        int pressedButton = -1;
        bool run(float dt) override;
    };
}

#endif
