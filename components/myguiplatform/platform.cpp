#include "platform.hpp"

#include <vsg/nodes/Node.h>

#include <components/vsgadapters/mygui/render.hpp>

#include "loglistener.hpp"
#include "vfs.hpp"

namespace MyGUIPlatform
{

    Platform::Platform(MyGUI::IntSize size, vsg::ref_ptr<vsgUtil::CompileContext> compile,
        const vsg::Options* imageOptions, const vsg::Options* shaderOptions, float uiScalingFactor,
        const VFS::Manager* in_vfs, const std::filesystem::path& _logName)
    {
        mLogManager = std::make_unique<MyGUI::LogManager>();
        if (!_logName.empty())
        {
            mLogFacility = std::make_unique<LogFacility>(_logName, false);
            mLogManager->addLogSource(mLogFacility->getSource());
        }
        mRenderManager = std::make_unique<vsgAdapters::mygui::Render>(
            size, compile, imageOptions, shaderOptions, uiScalingFactor);
        mDataManager = std::make_unique<vfs>(in_vfs, std::vector<std::string>{ "mygui", "fonts" });

        auto vertexShader = "gui/gui.vert";
        auto fragmentShader = "gui/gui.frag";
        mRenderManager->registerShader("", vertexShader, fragmentShader);
    }

    Platform::~Platform() {}

    void Platform::update(float dt)
    {
        mRenderManager->update(dt);
    }

    vsg::ref_ptr<vsg::Node> Platform::node()
    {
        return mRenderManager->node();
    }

}
