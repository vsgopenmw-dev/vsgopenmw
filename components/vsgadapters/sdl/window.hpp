#ifndef VSGOPENMW_VSGADAPTERS_SDL_WINDOW_H
#define VSGOPENMW_VSGADAPTERS_SDL_WINDOW_H

#include <vsg/viewer/WindowAdapter.h>

class SDL_Window;

namespace vsgAdapters::sdl
{
    // Renders into an existing SDL_Window created with SDL_WINDOW_VULKAN capability.
    class Window : public vsg::WindowAdapter
    {
    public:
        Window(SDL_Window *window, vsg::ref_ptr<vsg::WindowTraits> traits);
        const char* instanceExtensionSurfaceName() const override { return "VK_KHR_surface"; }

    protected:
        void _initSurface() override;
        SDL_Window *mWindow;
    };
}

#endif
