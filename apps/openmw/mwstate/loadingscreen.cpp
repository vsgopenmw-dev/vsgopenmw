#include "loadingscreen.hpp"

#include "../mwbase/windowmanager.hpp"
#include "../mwgui/loadingscreen.hpp"

namespace MWState
{
    LoadingScreen::LoadingScreen(std::shared_ptr<Loading> loading)
        : Gui(MWGui::GM_Loading)
        , mLoading(loading)
    {
        setWallpaper(true);
        requiresCompile = true;
        disableControls = true;
        disableEvents = true;
        showCursor = false;
    }

    bool LoadingScreen::run(float dt)
    {
        Gui::run(dt);
        if (!mLoading->run(dt))
        {
            quit();
            return false;
        }
        else if (hasQuitRequest())
            mLoading->abort = true;

        auto loadingScreen = static_cast<MWGui::LoadingScreen*>(mWm.getLoadingScreen());
        loadingScreen->offsetLabel = mWm.getMessagesCount()>0;
        loadingScreen->showWallpaper(wallpaper);
        loadingScreen->setComplete(mLoading->getComplete());
        loadingScreen->setLabel(mLoading->getDescription());
        return true;
    }
}
