#include "gui.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/statemanager.hpp"

namespace MWState
{
    Gui::Gui(MWGui::GuiMode gm)
        : mWm(*MWBase::Environment::get().getWindowManager())
        , mStateManager(*MWBase::Environment::get().getStateManager())
        , mGuiMode(gm)
    {
    }

    bool Gui::run(float dt)
    {
        if (!mPushedMode)
        {
            mWm.pushGuiMode(mGuiMode);
            mPushedMode = true;
        }
        else if (!mWm.containsMode(mGuiMode))
            return false;
        return true;
    }

    void Gui::quit()
    {
        mWm.popGuiMode(mGuiMode);
    }

    bool Gui::hasQuitRequest() const
    {
        return mStateManager.hasQuitRequest();
    }
}
