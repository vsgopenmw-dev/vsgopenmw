#ifndef VSGOPENMW_MWGUI_LOADINGSCREEN_H
#define VSGOPENMW_MWGUI_LOADINGSCREEN_H

#include "windowbase.hpp"

namespace VFS
{
    class Manager;
}
namespace MWGui
{
    class BackgroundImage;

    class LoadingScreen : public WindowBase
    {
    public:
        LoadingScreen(const VFS::Manager *vfs);
        bool stretch{};

        void setLabel (const std::string& label, bool offset);
        void update(float complete, float dt, bool showWallpaper);

    private:
        float mChangeWallpaperCountdown = 0;

        MyGUI::Widget* mLoadingBox;

        MyGUI::TextBox* mLoadingText;
        MyGUI::ScrollBar* mProgressBar;
        BackgroundImage* mSplashImage;

        std::vector<std::string> mSplashScreens;
    };
}


#endif
