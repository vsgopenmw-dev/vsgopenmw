#include "videowindow.hpp"

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

namespace MWGui
{
    VideoWindow::VideoWindow()
        : WindowBase("openmw_video.layout")
    {
        WindowBase::getWidget(mWidget, "Video");
    }

    void VideoWindow::playVideo (const std::string& video, const VFS::Manager &vfs, bool allowSkipping)
    {
        mWidget->playVideo(video, vfs);
        mWidget->eventKeyButtonPressed.clear();
        if (allowSkipping)
            mWidget->eventKeyButtonPressed += MyGUI::newDelegate(this, &VideoWindow::onKeyPressed);
    }

    void VideoWindow::onKeyPressed(MyGUI::Widget *_sender, MyGUI::KeyCode _key, MyGUI::Char _char)
    {
        if (_key == MyGUI::KeyCode::Escape)
            mWidget->stop();
    }
}
