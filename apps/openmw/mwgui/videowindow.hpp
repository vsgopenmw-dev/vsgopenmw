#ifndef VSGOPENMW_MWGUI_VIDEOWINDOW_H
#define VSGOPENMW_MWGUI_VIDEOWINDOW_H

#include "videowidget.hpp"
#include "windowbase.hpp"

namespace MWGui
{
    /*
     * Plays optionally skippable video.
     */
    class VideoWindow : public WindowBase
    {
        VideoWidget* mWidget;

    public:
        VideoWindow();

        void playVideo(const std::string& video, const VFS::Manager& vfs, bool allowSkipping);
        VideoWidget* getVideo() { return mWidget; }

        void onKeyPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
        MyGUI::Widget* getDefaultKeyFocus() override { return mWidget; }
    };
}

#endif
