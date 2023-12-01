#ifndef VSGOPENMW_SDLUTIL_INIT_H
#define VSGOPENMW_SDLUTIL_INIT_H

#include <SDL.h>
#include <SDL_config.h>

#include <iostream>
#include <stdexcept>

namespace SDLUtil
{

    void init()
    {
        SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0"); // We use only gamepads
        // Allows for Windows snapping features to properly work in borderless window
        SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "1");
        SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");

        Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
#if !SDL_JOYSTICK_DISABLED
        flags |= SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER;
#endif
#if !SDL_SENSOR_DISABLED
        flags |= SDL_INIT_SENSOR;
#endif
        if (SDL_WasInit(flags) == 0)
        {
            SDL_SetMainReady();
            if (SDL_Init(flags) != 0)
            {
                throw std::runtime_error("Could not initialize SDL! " + std::string(SDL_GetError()));
            }
        }
        SDL_version sdlVersion;
        SDL_GetVersion(&sdlVersion);
        std::cout << "SDL version: " << (int)sdlVersion.major << "." << (int)sdlVersion.minor << "."
                  << (int)sdlVersion.patch << std::endl;
    }

}

#endif
