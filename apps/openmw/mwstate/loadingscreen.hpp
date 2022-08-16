#ifndef VSGOPENMW_MWSTATE_LOADINGSCREEN_H
#define VSGOPENMW_MWSTATE_LOADINGSCREEN_H

#include "gui.hpp"
#include "loading.hpp"

namespace MWState
{
    /*
     * Displays progress.
     */
    class LoadingScreen : public Gui
    {
        std::shared_ptr<Loading> mLoading;
    public:
        LoadingScreen(std::shared_ptr<Loading> loading);
        void setWallpaper(bool show) { wallpaper = show; requiresScene = !show; }
        bool wallpaper = true;
        bool run(float dt) override;
    };

    template <class State, typename ... Args>
    std::shared_ptr<GameState> makeLoadingScreen(Args&& ... args)
    {
        return std::make_shared<LoadingScreen>(std::make_shared<State>(std::forward<Args&&>(args)...));
    }
}

#endif
