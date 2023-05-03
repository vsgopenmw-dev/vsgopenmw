#include "surface.hpp"

#include <array>
#include <stdexcept>

namespace vsgAdapters::sdl
{
    void fillSurface(SDL_Surface* surface, vsg::ref_ptr<vsg::Data> data)
    {
        int width = data->width();
        int height = data->height();
        if (surface->w != width || surface->h != height)
            throw std::runtime_error("surface->w != width || surface->h != height");

        auto srcFormat = data->properties.format;
        auto pixelOrder = SDL_PIXELORDER(surface->format->format);
        if (surface->format->BytesPerPixel == data->valueSize() && (
            ((pixelOrder == SDL_PACKEDORDER_ABGR || pixelOrder == SDL_PACKEDORDER_XBGR) && srcFormat == VK_FORMAT_R8G8B8A8_UNORM)
         || ((pixelOrder == SDL_PACKEDORDER_ARGB || pixelOrder == SDL_PACKEDORDER_XRGB) && srcFormat == VK_FORMAT_B8G8R8A8_UNORM)))
        {
            auto* pDst = (Uint8*)surface->pixels;
            auto* pSrc = (Uint8*)data->dataPointer();
            int srcPitch = data->valueSize() * width;
            if (srcPitch == surface->pitch)
                std::memcpy(pDst, pSrc, data->dataSize());
            else
            {
                for (int row = 0; row < height; ++row)
                {
                    std::memcpy(pDst, pSrc, srcPitch);
                    pSrc += srcPitch;
                    pDst += surface->pitch;
                }
            }
        }
        else
        {
            for (int x = 0; x < width; ++x)
            {
                for (int y = 0; y < height; ++y)
                {
                    std::array<Uint8, 4> clr;
                    auto* pixel = static_cast<Uint8*>(data->dataPointer(y * width + x));
                    switch (srcFormat)
                    {
                        case VK_FORMAT_R8G8B8A8_UNORM:
                        {
                            clr = { *pixel++, *pixel++, *pixel++, *pixel };
                            break;
                        }                                           case VK_FORMAT_B8G8R8A8_UNORM:
                        {
                            clr = { pixel[2], pixel[1], pixel[0], pixel[3] };
                            break;
                        }
                        default:
                            throw std::runtime_error("!SDL_MapRGBA(VkFormat=" + std::to_string(data->getLayout().format) + ")");
                    }
                    int bpp = surface->format->BytesPerPixel;
                    auto* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
                    *(Uint32*)(p) = SDL_MapRGBA(surface->format, clr[0], clr[1], clr[2], clr[3]);
                }
            }
        }
    }

    SurfaceUniquePtr createSurface(vsg::ref_ptr<vsg::Data> data)
    {
        int width = data->width();
        int height = data->height();
        SDL_Surface* surface
            = SDL_CreateRGBSurface(0, width, height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
        if (!surface)
            throw std::runtime_error(SDL_GetError());
        fillSurface(surface, data);
        return SurfaceUniquePtr(surface, SDL_FreeSurface);
    }
}
