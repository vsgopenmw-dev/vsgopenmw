#include "video.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

namespace MWState
{
    Video::Video(MWGui::VideoWindow *w)
        : Gui(MWGui::GM_Video)
        , mWindow(w)
    {
        disableControls = true;
        requiresScene = false;
        showCursor = false;
    }

    bool Video::run(float dt)
    {
        if (!mPushedMode && pauseSound && mWindow->getVideo()->hasAudioStream())
            MWBase::Environment::get().getSoundManager()->pauseSounds(MWSound::VideoPlayback, ~MWSound::Type::Movie & MWSound::Type::Mask);
        auto ret = Gui::run(dt) && mWindow->getVideo()->update() && !hasQuitRequest();
        if (!ret)
        {
            quit();
            mWindow->getVideo()->stop();
            MWBase::Environment::get().getSoundManager()->resumeSounds(MWSound::VideoPlayback);
        }
        return ret;
    }
}
