#ifndef VSGOPENMW_SDLUTIL_VIDEOWRAPPER_H
#define VSGOPENMW_SDLUTIL_VIDEOWRAPPER_H

#include <SDL_types.h>

struct SDL_Window;

namespace SDLUtil
{
    class VideoWrapper
    {
    public:
        VideoWrapper(SDL_Window* window);
        ~VideoWrapper();

        void setGammaContrast(float gamma, float contrast);

        void setVideoMode(int width, int height, bool fullscreen, bool windowBorder);

        void centerWindow();

    private:
        SDL_Window* mWindow;

        float mGamma = 1.f;
        float mContrast = 1.f;
        bool mHasSetGammaContrast = false;

        // Store system gamma ramp on window creation. Restore system gamma ramp on exit
        Uint16 mOldSystemGammaRamp[256*3];
    };
}

#endif
