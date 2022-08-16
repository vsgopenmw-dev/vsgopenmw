#include "choice.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWState
{
    bool Choice::run(float dt)
    {
        if (!mPushedDialog)
        {
            MWBase::Environment::get().getWindowManager()->interactiveMessageBox(message, buttons);
            mPushedDialog = true;
            return true;
        }
        else
        {
            pressedButton = MWBase::Environment::get().getWindowManager()->readPressedButton();
            return pressedButton == -1;
        }
    }
}
