#ifndef VSGOPENMW_RENDER_ENGINE_H
#define VSGOPENMW_RENDER_ENGINE_H

#include <memory>
#include <vector>
#include <functional>

#include <vsg/vk/vulkan.h>
#include <vsg/core/ref_ptr.h>

namespace vsg
{
    class Device;
    class DeviceFeatures;
    class PhysicalDevice;
    class ImageView;
    class CommandGraph;
    class SecondaryCommandGraph;
    class Window;
    class WindowTraits;
    class Data;
    class RenderGraph;
    class RenderPass;
    class Viewer;
}
namespace vsgUtil
{
    class DeletionQueue;
    class CompileContext;
    class DebugMessenger;
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
        Engine(vsg::ref_ptr<vsg::Window> window);
        using Callback = std::function<void(vsg::ref_ptr<vsg::Data> pixels)>;
        Engine(Callback headlessCallback, vsg::ref_ptr<vsg::WindowTraits> traits);
        ~Engine();

        bool useFrameQueue = true; // If set to false, prevents queued frames in the swapchain by inserting a vkDeviceWaitIdle call in between frames.

        void setup(std::vector<vsg::ref_ptr<vsg::CommandGraph>> graphs, bool threading);
        void compile();

        void updateExtents(const VkExtent2D& extent);
        VkSampleCountFlagBits framebufferSamples();
        void setRenderPass(vsg::ref_ptr<vsg::RenderPass> renderPass);

        using FrameCallback = std::function<void(double dt, bool& requiresRender, bool& quit)>;
        void loop(FrameCallback cb, float framerateLimit = 0);

        vsg::ref_ptr<Screenshot> screenshotOperation();
        size_t numFrames() const;
        vsg::ref_ptr<vsg::ImageView> colorImageView(size_t relativeFrameIndex);
        vsg::ref_ptr<vsg::ImageView> depthImageView();

        VkExtent2D extent2D() const;
        vsg::ref_ptr<vsg::PhysicalDevice> getOrCreatePhysicalDevice();
        vsg::ref_ptr<vsg::Device> getOrCreateDevice();
        vsg::ref_ptr<vsg::Device> getDevice();
        vsg::ref_ptr<vsgUtil::CompileContext> createCompileContext();
        vsg::ref_ptr<vsg::CommandGraph> createCommandGraph(int maxDescriptorSet);
        vsg::ref_ptr<vsg::SecondaryCommandGraph> createSecondaryCommandGraph(
            vsg::ref_ptr<vsg::RenderGraph> inherit_framebuffer, int maxDescriptorSet);
        vsg::ref_ptr<vsg::RenderGraph> createRenderGraph();
        const VkPhysicalDeviceFeatures& getEnabledFeatures() const;
        VkImageLayout getPresentLayout() const;

    private:
        vsg::ref_ptr<vsg::WindowTraits> mTraits;
        vsg::ref_ptr<vsg::Viewer> mViewer;
        vsg::ref_ptr<vsg::Window> mWindow;
        std::unique_ptr<Headless> mHeadless;
        std::unique_ptr<vsgUtil::DeletionQueue> mDeletionQueue;
        std::unique_ptr<vsgUtil::DebugMessenger> mDebugMessenger;
    };
}

#endif
