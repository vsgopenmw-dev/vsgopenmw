// vsgopenmw-unity-build

#include <MyGUI_ResourceManager.h>

#include <vsg/io/Options.h>

#include <components/sdlutil/cursormanager.hpp>
#include <components/vsgadapters/sdl/surface.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/resource/decompress.hpp>

#include "cursor.hpp"

namespace MWGui
{
    void createCursors(vsg::ref_ptr<const vsg::Options> imageOptions,
        SDLUtil::CursorManager* cursorManager)
    {
        auto decompressOptions = vsg::Options::create(*imageOptions);
        auto decompress = vsg::ref_ptr{ new Resource::Decompress };
        decompress->readerWriters = imageOptions->readerWriters;
        decompressOptions->readerWriters = { decompress };

        MyGUI::ResourceManager::EnumeratorPtr enumerator = MyGUI::ResourceManager::getInstance().getEnumerator();
        while (enumerator.next())
        {
            MyGUI::IResource* resource = enumerator.current().second;
            ResourceImageSetPointerFix* imgSetPointer = resource->castType<ResourceImageSetPointerFix>(false);
            if (!imgSetPointer)
                continue;
            auto tex_name = imgSetPointer->getImageSet()->getIndexInfo(0, 0).texture;
            MyGUI::IntSize pointerSize = imgSetPointer->getSize();
            if (auto data = vsgUtil::readOptionalImage(std::string(tex_name), decompressOptions))
            {
                auto surface = vsgAdapters::sdl::createSurface(data);
                Uint8 hotspot_x = imgSetPointer->getHotSpot().left;
                Uint8 hotspot_y = imgSetPointer->getHotSpot().top;
                int rotation = imgSetPointer->getRotation();
                cursorManager->createCursor(imgSetPointer->getResourceName(), rotation, surface.get(), hotspot_x,
                    hotspot_y, pointerSize.width, pointerSize.height);
            }
        }
    }
}
