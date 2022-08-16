//vsgopenmw-unity-build

#include <MyGUI_ResourceManager.h>

#include <components/sdlutil/cursormanager.hpp>
#include <components/vsgadapters/sdl/surface.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/render/download.hpp>

#include "cursor.hpp"

namespace MWGui
{
void createCursors(vsg::ref_ptr<vsg::Context> context, vsg::ref_ptr<const vsg::Options> imageOptions, SDLUtil::CursorManager *cursorManager)
{
    MyGUI::ResourceManager::EnumeratorPtr enumerator = MyGUI::ResourceManager::getInstance().getEnumerator();
    while (enumerator.next())
    {
        MyGUI::IResource* resource = enumerator.current().second;
        ResourceImageSetPointerFix* imgSetPointer = resource->castType<ResourceImageSetPointerFix>(false);
        if (!imgSetPointer)
            continue;
        std::string tex_name = imgSetPointer->getImageSet()->getIndexInfo(0,0).texture;

        auto data = vsgUtil::readOptionalImage(tex_name, imageOptions);
        if(data)
            data = Render::decompress(context, data);
        if(data)
        {
            auto surface = vsgAdapters::sdl::createSurface(data);
            Uint8 hotspot_x = imgSetPointer->getHotSpot().left;
            Uint8 hotspot_y = imgSetPointer->getHotSpot().top;
            int rotation = imgSetPointer->getRotation();
            cursorManager->createCursor(imgSetPointer->getResourceName(), rotation, surface.get(), hotspot_x, hotspot_y);
        }
    }
}
}
