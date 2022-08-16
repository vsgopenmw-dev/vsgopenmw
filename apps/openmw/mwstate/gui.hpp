#ifndef VSGOPENMW_MWSTATE_GUI_H
#define VSGOPENMW_MWSTATE_GUI_H

#include "gamestate.hpp"
#include "../mwgui/mode.hpp"

namespace MWBase
{
    class WindowManager;
    class StateManager;
}
namespace MWState
{
    /*
     * Pushes GuiMode.
     */
    class Gui : public GameState
    {
    public:
        Gui(MWGui::GuiMode gm);
        bool run(float dt) override;
    protected:
        MWBase::WindowManager &mWm;
        MWBase::StateManager &mStateManager;
        MWGui::GuiMode mGuiMode;
        bool mPushedMode = false;
        void quit();
        bool hasQuitRequest() const;
    };
}

#endif
