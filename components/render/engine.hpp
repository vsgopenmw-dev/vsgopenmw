#ifndef VSGOPENMW_RENDER_ENGINE_H
#define VSGOPENMW_RENDER_ENGINE_H

#include <vsg/vk/Device.h>
#include <vsg/viewer/CommandGraph.h>

namespace vsg
{
    class SecondaryCommandGraph;
    class CompileManager;
}
namespace Render
{
    class Screenshot;
    class Headless;

    /*
     * Encapsulates vsg::Viewer.
     */
    class Engine
    {
    public:
        Engine();
        ~Engine();
        void setWindow(vsg::ref_ptr<vsg::Window> window) { mWindow = window; }
        using Callback = std::function<void(vsg::ref_ptr<vsg::Data> pixels)>;
        void setHeadless(Callback callback, vsg::ref_ptr<vsg::WindowTraits> traits);

        void setup(vsg::CommandGraphs graphs);
        void compile();

        void updateExtents(const VkExtent2D &extent);

        using FrameCallback = std::function<void(double dt, bool &requiresRender, bool &quit)>;
        void loop(FrameCallback cb, float framerateLimit=0);

        vsg::ref_ptr<Screenshot> screenshotOperation();

        VkExtent2D extent2D() const;
        vsg::ref_ptr<vsg::Device> getOrCreateDevice();
        vsg::ref_ptr<vsg::CompileManager> createCompileManager();
        vsg::ref_ptr<vsg::CommandGraph> createCommandGraph(int maxDescriptorSet);
        vsg::ref_ptr<vsg::SecondaryCommandGraph> createSecondaryCommandGraph(vsg::ref_ptr<vsg::RenderGraph> inherit_framebuffer, int maxDescriptorSet);
        vsg::ref_ptr<vsg::RenderGraph> createRenderGraph();
    private:
        vsg::ref_ptr<vsg::Viewer> mViewer;
        vsg::ref_ptr<vsg::Window> mWindow;
        std::unique_ptr<Headless> mHeadless;
    };
}

#endif
