#include "manualtexture.hpp"

#include <MyGUI_RenderManager.h>

namespace vsgAdapters::mygui
{
    MyGUI::PixelFormat format(VkFormat vk)
    {
        switch (vk)
        {
            case VK_FORMAT_R8G8B8A8_UNORM:
                return MyGUI::PixelFormat::R8G8B8A8;
            case VK_FORMAT_R8G8B8_UNORM:
                return MyGUI::PixelFormat::R8G8B8;
            default:
               throw std::runtime_error("!MyGUI::PixelFormat(VkFormat(" + std::to_string(vk));
        }
    }

    bool compatible(MyGUI::ITexture *tex, vsg::ref_ptr<vsg::Data> data)
    {
        auto pf = format(data->getLayout().format);
        return tex->getFormat() == pf && static_cast<uint32_t>(tex->getWidth()) == data->width() && static_cast<uint32_t>(tex->getHeight()) == data->height();
    }

    void fillManualTexture(MyGUI::ITexture *tex, vsg::ref_ptr<vsg::Data> data)
    {
        if (compatible(tex, data))
        {
            auto *dst = reinterpret_cast<unsigned char*>(tex->lock(MyGUI::TextureUsage::Write));
            std::memcpy(dst, data->dataPointer(), data->dataSize());
            tex->unlock();
        }
    }

    MyGUI::ITexture* createManualTexture(const std::string &name, vsg::ref_ptr<vsg::Data> data)
    {
        auto texture = MyGUI::RenderManager::getInstance().createTexture(name);
        texture->createManual(data->width(), data->height(), MyGUI::TextureUsage::Write, format(data->getLayout().format));
        fillManualTexture(texture, data);
        return texture;
    }

    void createOrUpdateManualTexture(MyGUI::ITexture *&tex, const std::string &name, vsg::ref_ptr<vsg::Data> data)
    {
        if (tex && !compatible(tex, data))
        {
            MyGUI::RenderManager::getInstance().destroyTexture(tex);
            tex = {};
        }
        if (!tex)
            tex = createManualTexture(name, data);
        fillManualTexture(tex, data);
    }
}
