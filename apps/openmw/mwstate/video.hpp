#ifndef VSGOPENMW_MWSTATE_VIDEO_H
#define VSGOPENMW_MWSTATE_VIDEO_H

#include "gui.hpp"
#include "../mwgui/videowindow.hpp"

namespace MWState
{
    /*
     * Exclusively plays video.
     */
    class Video : public Gui
    {
        MWGui::VideoWindow *mWindow;
    public:
        bool pauseSound = true;
        Video(MWGui::VideoWindow *w);
        bool run(float dt) override;
    };
}

#endif
