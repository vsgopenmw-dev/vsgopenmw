#ifndef OPENMW_MWGUI_VIDEOWIDGET_H
#define OPENMW_MWGUI_VIDEOWIDGET_H

#include <memory>

#include "backgroundimage.hpp"

namespace Video
{
    class VideoPlayer;
}

namespace VFS
{
    class Manager;
}

namespace MWGui
{

    /**
     * Plays video.
     */
    class VideoWidget : public BackgroundImage
    {
        MYGUI_RTTI_DERIVED(VideoWidget)
    public:
        VideoWidget();
        ~VideoWidget();

        /// Stretch the video to fill the whole screen? If false, black bars may be added to fix the aspect ratio.
        bool stretch = false;

        void playVideo (const std::string& video, const VFS::Manager &vfs);

        /// @return Is the video still playing?
        bool update();

        /// Stop video and free resources (done automatically on destruction)
        void stop();

        /// Return true if a video is currently playing and it has an audio stream.
        bool hasAudioStream();
    private:
        MyGUI::ITexture *mTexture = nullptr;
        std::unique_ptr<Video::VideoPlayer> mPlayer;
    };
}

#endif
