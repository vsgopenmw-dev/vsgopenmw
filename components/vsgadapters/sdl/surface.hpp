#ifndef VSGOPENMW_VSGADAPTERS_SDL_SURFACE_H
#define VSGOPENMW_VSGADAPTERS_SDL_SURFACE_H

#include <memory>
#include <iostream>
#include <cassert>

#include <vsg/core/Data.h>

#include <SDL_surface.h>

namespace vsgAdapters {
namespace sdl
{
typedef std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)> SurfaceUniquePtr;

inline void fillSurface(SDL_Surface *surface, vsg::ref_ptr<vsg::Data> data, bool flip)
{
    int width = data->width();
    int height = data->height();
    if (surface->w != width || surface->h != height)
        return;
    for(int x = 0; x < width; ++x)
        for(int y = 0; y < height; ++y)
        {
            int tx = x;
            int ty = flip ? (height-1)-y : y;
            std::array<Uint8, 4> clr;
            switch (data->getLayout().format)
            {
                case VK_FORMAT_R8G8B8A8_UNORM:
                {
                    auto *pixel = static_cast<Uint8*>(data->dataPointer(ty*width+tx));
                    clr = {*pixel++, *pixel++, *pixel++, *pixel};
                    break;
                }
                default:
                    std::cerr << "!surface(VkFormat(" << data->getLayout().format << "))" << std::endl;
            }
            int bpp = surface->format->BytesPerPixel;
            auto *p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
            *(Uint32*)(p) = SDL_MapRGBA(surface->format, clr[0], clr[1], clr[2], clr[3]);
        }
}

inline SurfaceUniquePtr createSurface(vsg::ref_ptr<vsg::Data> data)
{
    int width = data->width();
    int height = data->height();
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
    fillSurface(surface, data, true);
    return SurfaceUniquePtr(surface, SDL_FreeSurface);
}

}}

#endif
