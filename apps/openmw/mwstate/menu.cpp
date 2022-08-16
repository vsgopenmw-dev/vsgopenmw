#include "menu.hpp"

namespace MWState
{
    Menu::Menu()
        : Gui(MWGui::GM_MainMenu)
    {
    }

    bool Menu::run(float dt)
    {
        return !hasQuitRequest() && Gui::run(dt);
    }
}
