#ifndef VSGOPENMW_MYGUI_PLATFORM_H
#define VSGOPENMW_MYGUI_PLATFORM_H

#include <string>
#include <memory>

#include <vsg/core/ref_ptr.h>
#include <MyGUI_Types.h>

namespace vsg
{
    class Node;
    class Options;
    class Context;
}
namespace vsgAdapters {
namespace mygui {
    class Render;
}}
namespace VFS
{
    class Manager;
}
namespace MyGUI
{
    class LogManager;
    class DataManager;
}
namespace MyGUIPlatform
{
    class LogFacility;

    class Platform
    {
    public:
        Platform(MyGUI::IntSize size, vsg::Context *context, const vsg::Options *options, float uiScalingFactor, const VFS::Manager *in_vfs, const std::string& shaderPath, const std::string& _logName);
        ~Platform();

        vsg::ref_ptr<vsg::Node> node();
        void update(float dt);
    private:
        std::unique_ptr<LogFacility> mLogFacility;
        std::unique_ptr<MyGUI::LogManager> mLogManager;
        std::unique_ptr<MyGUI::DataManager> mDataManager;
        std::unique_ptr<vsgAdapters::mygui::Render> mRenderManager;

        void operator=(const Platform&);
        Platform(const Platform&);
    };
}

#endif
