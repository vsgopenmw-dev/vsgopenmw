#ifndef VSGOPENMW_MWSTATE_LOADINGSCREEN_H
#define VSGOPENMW_MWSTATE_LOADINGSCREEN_H

#include <memory>

#include "gui.hpp"
#include "loading.hpp"

namespace MWGui
{
    class LoadingScreen;
}
namespace MWState
{
    /*
     * Informatively runs loading.
     */
    class LoadingScreen : public Gui
    {
        MWGui::LoadingScreen* mWindow;

    public:
        LoadingScreen(MWGui::LoadingScreen* window);
        std::shared_ptr<Loading> loading;
        void setWallpaper(bool show)
        {
            wallpaper = show;
            requiresScene = !show;
            if (show)
                showCursor = false;
            else
                showCursor = std::nullopt;
        }
        bool wallpaper = true;
        bool run(float dt) override;
    };
}

#endif
