#ifndef VSGOPENMW_MYGUI_PLATFORM_H
#define VSGOPENMW_MYGUI_PLATFORM_H

#include <filesystem>
#include <memory>
#include <string>

#include <MyGUI_Types.h>
#include <vsg/core/ref_ptr.h>

#include <components/vsgutil/compilecontext.hpp>

namespace vsg
{
    class Node;
    class Options;
}
namespace vsgAdapters::mygui
{
    class Render;
}
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
        Platform(MyGUI::IntSize size, const vsg::ref_ptr<vsgUtil::CompileContext> compile,
            const vsg::Options* imageOptions, const vsg::Options* shaderOptions, float uiScalingFactor,
            const VFS::Manager* in_vfs, const std::filesystem::path& _logName);
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
