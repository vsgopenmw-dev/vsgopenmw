#ifndef VSGOPENMW_MWSTATE_VIDEO_H
#define VSGOPENMW_MWSTATE_VIDEO_H

#include "../mwgui/videowindow.hpp"
#include "gui.hpp"

namespace MWState
{
    /*
     * Exclusively plays video.
     */
    class Video : public Gui
    {
        MWGui::VideoWindow* mWindow;

    public:
        bool pauseSound = true;
        Video(MWGui::VideoWindow* w);
        bool run(float dt) override;
    };
}

#endif
