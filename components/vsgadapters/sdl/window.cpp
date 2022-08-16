#include "window.hpp"

#include <SDL_vulkan.h>

namespace vsgAdapters {
namespace sdl
{
    Window::Window(SDL_Window *window, vsg::ref_ptr<vsg::WindowTraits> traits)
        : vsg::WindowAdapter(vsg::ref_ptr<vsg::Surface>(), traits)
        , mWindow(window)
    {
        unsigned int extensionCount;
        const char** extensionNames = 0;
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
        extensionNames = new const char *[extensionCount];
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames);

        for (unsigned int i=0; i<extensionCount; ++i)
            _traits->instanceExtensionNames.push_back(extensionNames[i]);
        windowVisible = true;
        windowValid = true;
    }

    void Window::_initSurface()
    {
        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(mWindow, _instance->getInstance(), &surface))
            throw std::runtime_error("SDL_Vulkan_CreateSurface failed");
        _surface = vsg::Surface::create(surface, _instance);
    }
}}
