#include "menu.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/soundmanager.hpp"

namespace MWState
{
    Menu::Menu()
        : Gui(MWGui::GM_MainMenu)
    {
    }

    bool Menu::run(float dt)
    {
        auto state = MWBase::Environment::get().getStateManager()->getState();
        auto soundMgr = MWBase::Environment::get().getSoundManager();

        if (state == MWBase::StateManager::State_NoGame && !soundMgr->isMusicPlaying())
        {
            std::string titlefile = "music/special/morrowind title.mp3";
            //if (mVFS->exists(titlefile))
            streamMusic(titlefile, MWSound::MusicType::Special);
        }

        return !hasQuitRequest() && Gui::run(dt);
    }
}
