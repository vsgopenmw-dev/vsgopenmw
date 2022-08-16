#include "loadingscreen.hpp"

#include <array>

#include <MyGUI_ScrollBar.h>
#include <MyGUI_TextBox.h>

#include <components/misc/pathhelpers.hpp>
#include <components/misc/rng.hpp>
#include <components/vfs/manager.hpp>

#include "backgroundimage.hpp"

namespace MWGui
{
    LoadingScreen::LoadingScreen(const VFS::Manager *vfs)
        : WindowBase("openmw_loading_screen.layout")
    {
        getWidget(mLoadingText, "LoadingText");
        getWidget(mProgressBar, "ProgressBar");
        getWidget(mLoadingBox, "LoadingBox");
        getWidget(mSplashImage, "Splash");

        mProgressBar->setScrollViewPage(1);

        auto isSupportedExtension = [](const std::string_view& ext) {
            static const std::array<std::string, 7> supported_extensions{ {"tga", "dds", "ktx", "png", "bmp", "jpeg", "jpg"} };
            return !ext.empty() && std::find(supported_extensions.begin(), supported_extensions.end(), ext) != supported_extensions.end();
        };
        for (const auto& name : vfs->getRecursiveDirectoryIterator("Splash/"))
        {
            if (isSupportedExtension(Misc::getFileExtension(name)))
                mSplashScreens.push_back(name);
        }
    }

    LoadingScreen::~LoadingScreen()
    {
    }

    void LoadingScreen::setLabel(const std::string &label)
    {
        if (!label.empty())
        {
            mLoadingText->setCaptionWithReplacing(label);
            int padding = mLoadingBox->getWidth() - mLoadingText->getWidth();
            MyGUI::IntSize size(mLoadingText->getTextSize().width+padding, mLoadingBox->getHeight());
            size.width = std::max(300, size.width);
            mLoadingBox->setSize(size);

            if (offsetLabel)
                mLoadingBox->setPosition(mMainWidget->getWidth()/2 - mLoadingBox->getWidth()/2, mMainWidget->getHeight()/2 - mLoadingBox->getHeight()/2);
            else
                mLoadingBox->setPosition(mMainWidget->getWidth()/2 - mLoadingBox->getWidth()/2, mMainWidget->getHeight() - mLoadingBox->getHeight() - 8);
            mLoadingBox->setVisible(true);
        }
        else
            mLoadingBox->setVisible(false);
    }

    void LoadingScreen::setVisible(bool visible)
    {
        WindowBase::setVisible(visible);
        mSplashImage->setVisible(visible && mShowWallpaper);
        if (visible)
            changeWallpaperIfRequired();
    }

    void LoadingScreen::showWallpaper(bool show)
    {
        mShowWallpaper = show;
        mSplashImage->setVisible(mMainWidget->getVisible() && show);
    }

    void LoadingScreen::changeWallpaperIfRequired()
    {
        if (!mSplashScreens.empty() && mShowWallpaper && mNextWallpaperChangeTime <= mTimer.time_m())
        {
            mNextWallpaperChangeTime = mTimer.time_m() + 5000;
            std::string const & randomSplash = mSplashScreens.at(Misc::Rng::rollDice(mSplashScreens.size()));

            // TODO: add option (filename pattern?) to use image aspect ratio instead of 4:3
            // we can't do this by default, because the Morrowind splash screens are 1024x1024, but should be displayed as 4:3
            mSplashImage->setBackgroundImage(randomSplash, true, stretch);
        }
    }

    void LoadingScreen::setComplete(float fraction)
    {
        changeWallpaperIfRequired();
        size_t resolution = 1000;
        mProgressBar->setScrollRange(resolution+1);
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(fraction * mProgressBar->getLineSize());
    }
}
