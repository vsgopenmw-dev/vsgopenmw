#ifndef OPENMW_GAME_MWGUI_MAINMENU_H
#define OPENMW_GAME_MWGUI_MAINMENU_H

#include <memory>

#include "windowbase.hpp"

namespace Gui
{
    class ImageButton;
}

namespace VFS
{
    class Manager;
}

namespace MWGui
{
    class BackgroundImage;
    class SaveGameDialog;
    class VideoWidget;

    class MainMenu : public WindowBase
    {
        int mWidth;
        int mHeight;

        bool mHasAnimatedMenu;

    public:
        MainMenu(int w, int h, const VFS::Manager* vfs, const std::string& versionDescription);
        ~MainMenu();

        void onResChange(int w, int h) override;

        void setVisible(bool visible) override;

        void onFrame(float dt) override;

        bool exit() override;
        bool stretch{};

    private:
        const VFS::Manager* mVFS;

        MyGUI::Widget* mButtonBox;
        MyGUI::TextBox* mVersionText;

        BackgroundImage* mBackground;
        VideoWidget* mVideo; // For animated main menus

        std::map<std::string, Gui::ImageButton*, std::less<>> mButtons;

        void onButtonClicked(MyGUI::Widget* sender);
        void onNewGameConfirmed();
        void onExitConfirmed();

        void showBackground(bool show);

        void updateMenu();

        std::unique_ptr<SaveGameDialog> mSaveGameDialog;
    };

}

#endif
