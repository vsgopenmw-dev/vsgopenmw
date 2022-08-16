#ifndef VSGOPENMW_MWGUI_LOADINGSCREEN_H
#define VSGOPENMW_MWGUI_LOADINGSCREEN_H

#include <osg/Timer>

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
        ~LoadingScreen();
        bool stretch{};
        bool offsetLabel{};

        void setLabel (const std::string& label);

        void setComplete(float fraction);

        void setVisible(bool visible) override;
        void showWallpaper(bool show);

    private:
        void changeWallpaperIfRequired();

        double mNextWallpaperChangeTime = 0.0;

        bool mShowWallpaper = true;
        osg::Timer mTimer;

        MyGUI::Widget* mLoadingBox;

        MyGUI::TextBox* mLoadingText;
        MyGUI::ScrollBar* mProgressBar;
        BackgroundImage* mSplashImage;

        std::vector<std::string> mSplashScreens;
    };
}


#endif
