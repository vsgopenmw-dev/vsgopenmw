#include "platform.hpp"

#include <vsg/nodes/Node.h>

#include <components/vsgadapters/mygui/render.hpp>

#include "vfs.hpp"
#include "loglistener.hpp"

namespace MyGUIPlatform
{

Platform::Platform(MyGUI::IntSize size, vsg::Context *context, const vsg::Options *options, float uiScalingFactor, const VFS::Manager *in_vfs, const std::string &shaderPath, const std::string &_logName)
{
    mLogManager = std::make_unique<MyGUI::LogManager>();
    if (!_logName.empty())
    {
        mLogFacility = std::make_unique<LogFacility>(_logName, false);
        mLogManager->addLogSource(mLogFacility->getSource());
    }
    mRenderManager = std::make_unique<vsgAdapters::mygui::Render>(size, context, options, uiScalingFactor);
    mDataManager = std::make_unique<vfs>(in_vfs, std::vector<std::string>{"mygui","fonts"});

    auto vertexShader = shaderPath + "/gui.vert";
    auto fragmentShader = shaderPath + "/gui.frag";
    mRenderManager->registerShader("", vertexShader, fragmentShader);
}

Platform::~Platform()
{
}

void Platform::update(float dt)
{
    mRenderManager->update(dt);
}

vsg::ref_ptr<vsg::Node> Platform::node()
{
    return mRenderManager->node();
}

}
