#ifndef VSGOPENMW_VSGADAPTERS_SDL_SURFACE_H
#define VSGOPENMW_VSGADAPTERS_SDL_SURFACE_H

#include <memory>

#include <vsg/core/Data.h>

#include <SDL_surface.h>

namespace vsgAdapters::sdl
{
    typedef std::unique_ptr<SDL_Surface, void (*)(SDL_Surface*)> SurfaceUniquePtr;

    void fillSurface(SDL_Surface* surface, vsg::ref_ptr<vsg::Data> data);

    SurfaceUniquePtr createSurface(vsg::ref_ptr<vsg::Data> data);
}

#endif
