#ifndef VSGOPENMW_VSGADAPTERS_MYGUI_MANUALTEXTURE_H
#define VSGOPENMW_VSGADAPTERS_MYGUI_MANUALTEXTURE_H

#include <MyGUI_ITexture.h>

#include <vsg/core/Data.h>

namespace vsgAdapters::mygui
{
    void fillManualTexture(MyGUI::ITexture *tex, vsg::ref_ptr<vsg::Data> data);
    MyGUI::ITexture* createManualTexture(const std::string &name, vsg::ref_ptr<vsg::Data> data);
    void createOrUpdateManualTexture(MyGUI::ITexture *&tex, const std::string &name, vsg::ref_ptr<vsg::Data> data);
}

#endif
