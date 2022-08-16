#include "cursormanager.hpp"

#include <memory>

#include <SDL_mouse.h>
#include <SDL_endian.h>
#include <SDL_render.h>
#include <SDL_hints.h>

namespace
{
    typedef std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)> SurfaceUniquePtr;
    SurfaceUniquePtr rotate(SDL_Surface *cursorSurface, float rotDegrees)
    {
        int width = cursorSurface->w;
        int height = cursorSurface->h;

        Uint32 redMask = 0x000000ff;
        Uint32 greenMask = 0x0000ff00;
        Uint32 blueMask = 0x00ff0000;
        Uint32 alphaMask = 0xff000000;

        SDL_Surface *targetSurface = SDL_CreateRGBSurface(0, width, height, 32, redMask, greenMask, blueMask, alphaMask);
        SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(targetSurface);

        SDL_RenderClear(renderer);

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        SDL_Texture *cursorTexture = SDL_CreateTextureFromSurface(renderer, cursorSurface);

        SDL_RenderCopyEx(renderer, cursorTexture, nullptr, nullptr, -rotDegrees, nullptr, SDL_FLIP_NONE);

        SDL_DestroyTexture(cursorTexture);
        SDL_DestroyRenderer(renderer);

        return SurfaceUniquePtr(targetSurface, SDL_FreeSurface);
    }
}

namespace SDLUtil
{
    CursorManager::~CursorManager()
    {
        CursorMap::const_iterator curs_iter = mCursorMap.begin();

        while(curs_iter != mCursorMap.end())
        {
            SDL_FreeCursor(curs_iter->second);
            ++curs_iter;
        }

        mCursorMap.clear();
    }

    void CursorManager::cursorChanged(const std::string& name)
    {
        auto it = mCursorMap.find(name);
        if (it != mCursorMap.end())
            SDL_SetCursor(it->second);
    }

    void CursorManager::createCursor(const std::string& name, int rotDegrees, SDL_Surface *surface, Uint8 hotspot_x, Uint8 hotspot_y)
    {
        if (mCursorMap.find(name) != mCursorMap.end())
            return;
        SDL_Cursor *curs;
        if (rotDegrees == 0)
            curs = SDL_CreateColorCursor(surface, hotspot_x, hotspot_y);
        else
        {
            auto rotated = rotate(surface, static_cast<float>(rotDegrees));
            curs = SDL_CreateColorCursor(rotated.get(), hotspot_x, hotspot_y);
        }
        mCursorMap.insert(CursorMap::value_type(std::string(name), curs));
    }
}
