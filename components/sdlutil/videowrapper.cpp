#include "videowrapper.hpp"

#include <iostream>
#include <algorithm>

#include <SDL_video.h>

namespace SDLUtil
{
    VideoWrapper::VideoWrapper(SDL_Window* window)
        : mWindow(window)
    {
        SDL_GetWindowGammaRamp(mWindow, mOldSystemGammaRamp, &mOldSystemGammaRamp[256], &mOldSystemGammaRamp[512]);
    }

    VideoWrapper::~VideoWrapper()
    {
        SDL_SetWindowFullscreen(mWindow, 0);

        // If user hasn't touched the defaults no need to restore
        if (mHasSetGammaContrast)
            SDL_SetWindowGammaRamp(mWindow, mOldSystemGammaRamp, &mOldSystemGammaRamp[256], &mOldSystemGammaRamp[512]);
    }

    void VideoWrapper::setGammaContrast(float gamma, float contrast)
    {
        if (gamma == mGamma && contrast == mContrast)
            return;

        mGamma = gamma;
        mContrast = contrast;

        mHasSetGammaContrast = true;

        Uint16 red[256], green[256], blue[256];
        for (int i = 0; i < 256; i++)
        {
            float k = i / 256.0f;
            k = (k - 0.5f) * contrast + 0.5f;
            k = pow(k, 1.f / gamma);
            k *= 256;
            float value = std::clamp(k * 256, 0.f, 65535.f);
            red[i] = green[i] = blue[i] = static_cast<Uint16>(value);
        }
        if (SDL_SetWindowGammaRamp(mWindow, red, green, blue) < 0)
            std::cerr << "Couldn't set gamma: " << SDL_GetError() << std::endl;
    }

    void VideoWrapper::setVideoMode(int width, int height, bool fullscreen, bool windowBorder)
    {
        SDL_SetWindowFullscreen(mWindow, 0);

        if (SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MAXIMIZED)
            SDL_RestoreWindow(mWindow);

        if (fullscreen)
        {
            SDL_DisplayMode mode;
            SDL_GetWindowDisplayMode(mWindow, &mode);
            mode.w = width;
            mode.h = height;
            SDL_SetWindowDisplayMode(mWindow, &mode);
            SDL_SetWindowFullscreen(mWindow, SDL_WINDOW_FULLSCREEN /* : SDL_WINDOW_FULLSCREEN_DESKTOP*/);
        }
        else
        {
            SDL_SetWindowSize(mWindow, width, height);
            SDL_SetWindowBordered(mWindow, windowBorder ? SDL_TRUE : SDL_FALSE);
            centerWindow();
        }
    }

    void VideoWrapper::centerWindow()
    {
        // Resize breaks the sdl window in some cases; see issue: #5539
        SDL_Rect rect{};
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        auto index = SDL_GetWindowDisplayIndex(mWindow);
        SDL_GetDisplayBounds(index, &rect);
        SDL_GetWindowSize(mWindow, &w, &h);

        x = rect.x;
        y = rect.y;

        // Center dimensions that do not fill the screen
        if (w < rect.w)
        {
            x = rect.x + rect.w / 2 - w / 2;
        }
        if (h < rect.h)
        {
            y = rect.y + rect.h / 2 - h / 2;
        }
        SDL_SetWindowPosition(mWindow, x, y);
    }
}
