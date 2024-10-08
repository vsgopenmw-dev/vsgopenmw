#include "error.hpp"

#include <iostream>

#include "../mwbase/windowmanager.hpp"

namespace MWState
{
    Error::Error(const std::string& msg)
        : mMessage(msg)
    {
        std::cerr << msg << std::endl;
    }

    bool Error::run(float dt)
    {
        bool ret = Menu::run(dt);
        if (!mMessage.empty())
        {
            mWm.interactiveMessageBox(mMessage, { "#{Interface:OK}" });
            mChoiceActive = true;
            mMessage.clear();
        }
        if (mChoiceActive && mWm.readPressedButton() != -1)
            mChoiceActive = false;
        return ret;
    }
}
