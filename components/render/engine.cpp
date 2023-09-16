#include "engine.hpp"

#include <iostream>

#include <vsg/app/RenderGraph.h>
#include <vsg/app/SecondaryCommandGraph.h>
#include <vsg/app/Viewer.h>
#include <vsg/app/WindowAdapter.h>
#include <vsg/commands/Commands.h>
#include <vsg/core/Exception.h>
#include <vsg/vk/Device.h>

#include <components/vsgutil/deletionqueue.hpp>
#include <components/vsgutil/compilecontext.hpp>
#include <components/vsgutil/debugmessenger.hpp>

#include "headless.hpp"
#include "limitframerate.hpp"
#include "screenshot.hpp"

namespace Render
{
    Engine::Engine(vsg::ref_ptr<vsg::Window> window)
        : mWindow(window)
    {
        mViewer = vsg::Viewer::create();
        mViewer->addWindow(mWindow);
        mTraits = mWindow->traits();
        mDeletionQueue = std::make_unique<vsgUtil::DeletionQueue>(mTraits->swapchainPreferences.imageCount+1);
        if (mTraits->debugLayer) mDebugMessenger = std::make_unique<vsgUtil::DebugMessenger>(mWindow->getOrCreateInstance());
    }

    Engine::Engine(Callback headlessCallback, vsg::ref_ptr<vsg::WindowTraits> traits)
    {
        mHeadless = std::make_unique<Headless>(headlessCallback, traits);
        mViewer = vsg::Viewer::create();
        mTraits = traits;
        //mDeletionQueue = {};
        mDeletionQueue = std::make_unique<vsgUtil::DeletionQueue>(3);
        if (mTraits->debugLayer) mDebugMessenger = std::make_unique<vsgUtil::DebugMessenger>(mHeadless->getOrCreateInstance());
    }

    Engine::~Engine() {}

    void Engine::setup(vsg::CommandGraphs graphs, bool threading)
    {
        if (mDeletionQueue && mWindow)
        {
            mWindow->getOrCreateSwapchain();
            mDeletionQueue->resize(mWindow->numFrames());
        }
        else if (mHeadless)
        {
            auto commandGraph = createCommandGraph(0);
            commandGraph->children = { mHeadless->commands };
            commandGraph->submitOrder = std::numeric_limits<int>::max();
            graphs.push_back(commandGraph);
        }

        mViewer->assignRecordAndSubmitTaskAndPresentation(graphs);
        if (threading)
            mViewer->setupThreading();
    }

    void Engine::compile()
    {
        try
        {
            auto start = std::chrono::steady_clock::now();
            mViewer->compile();
            auto end = std::chrono::steady_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
            std::cout <<"Engine::compile() = " << dt << "s" << std::endl;
        }
        catch (const vsg::Exception& e)
        {
            auto message = e.message + " VkResult " + std::to_string(e.result);
            std::cerr << message << std::endl;
            throw std::runtime_error(message);
        }
    }

    void Engine::loop(FrameCallback frameCallback, float framerateLimit)
    {
        auto limitFramerate = LimitFramerate::create(framerateLimit);
        auto last = std::chrono::steady_clock::now();
        while (mViewer->advanceToNextFrame())
        {
            auto now = std::chrono::steady_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(now - last).count();
            last = now;

            static float fpsTimer = 0;
            fpsTimer += dt;
            if (fpsTimer > 5)
            {
                std::cout << "FPS: " << 1.0 / dt << std::endl;
                fpsTimer = 0;
            }

            mViewer->handleEvents();

            bool requiresRender = true;
            bool quit = false;
            do
            {
                frameCallback(dt, requiresRender, quit);
                /*
                 * Discards time taken by non-rendering frames to prevent large values of dt.
                 */
                if (!requiresRender)
                    last = std::chrono::steady_clock::now();
            }
            while (!requiresRender && !quit);

            if (quit)
                break;

            mViewer->update();
            mViewer->recordAndSubmit();

            if (mHeadless)
            {
                uint64_t waitTimeout = 19999999999;
                mViewer->waitForFences(0, waitTimeout);
                mHeadless->copyOutput();
            }
            else
                mViewer->present();

            if (!useFrameQueue)
                mViewer->deviceWaitIdle();

            if (mDeletionQueue)
                mDeletionQueue->advanceFrame();

            limitFramerate.limit();
        }

        // Wait for GPU before attempting to delete any vulkan objects.
        mViewer->deviceWaitIdle();
    }

    VkExtent2D Engine::extent2D() const
    {
        if (mWindow)
            return mWindow->extent2D();
        else
            return mHeadless->extent2D();
    }

    vsg::ref_ptr<vsg::PhysicalDevice> Engine::getOrCreatePhysicalDevice()
    {
        if (mWindow)
            return mWindow->getOrCreatePhysicalDevice();
        else
            return mHeadless->getOrCreatePhysicalDevice();
    }

    vsg::ref_ptr<vsg::Device> Engine::getDevice()
    {
        if (mWindow)
            return mWindow->getDevice();
        else
            return mHeadless->getDevice();
    }

    vsg::ref_ptr<vsg::Device> Engine::getOrCreateDevice()
    {
        if (mWindow)
            return mWindow->getOrCreateDevice();
        else
            return mHeadless->getOrCreateDevice();
    }

    vsg::ref_ptr<vsg::CommandGraph> Engine::createCommandGraph(int maxDescriptorSet)
    {
        vsg::ref_ptr<vsg::CommandGraph> graph;
        if (mWindow)
            graph = vsg::CommandGraph::create(mWindow);
        else
            graph = vsg::CommandGraph::create(mHeadless->getOrCreateDevice(), mHeadless->getQueueFamily());
        //int maxSlot = 1 + maxDescriptorSet;
        int maxSlot = 4; // At the moment, reducing maxSlot won't help because vsg::updateViewer will update the maxSlot for all command graphs.
        graph->recordTraversal = vsg::RecordTraversal::create(nullptr, maxSlot);
        graph->maxSlot = maxSlot;
        return graph;
    }

    vsg::ref_ptr<vsg::SecondaryCommandGraph> Engine::createSecondaryCommandGraph(
        vsg::ref_ptr<vsg::RenderGraph> inherit_framebuffer, int maxDescriptorSet)
    {
        inherit_framebuffer->contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
        vsg::ref_ptr<vsg::SecondaryCommandGraph> graph;
        if (mWindow)
            graph = vsg::SecondaryCommandGraph::create(mWindow);
        else
        {
            graph = vsg::SecondaryCommandGraph::create(mHeadless->getOrCreateDevice(), mHeadless->getQueueFamily());
            graph->framebuffer = inherit_framebuffer->framebuffer;
        }
        graph->recordTraversal = vsg::RecordTraversal::create(nullptr, maxDescriptorSet + 1);
        return graph;
    }

    vsg::ref_ptr<vsg::RenderGraph> Engine::createRenderGraph()
    {
        vsg::ref_ptr<vsg::RenderGraph> renderGraph;
        if (mWindow)
            renderGraph = vsg::RenderGraph::create(mWindow);
        else
            renderGraph = mHeadless->createRenderGraph();
        /*if (useDynamicViewport)*/ renderGraph->windowResizeHandler = nullptr;
        return renderGraph;
    }

    vsg::ref_ptr<vsgUtil::CompileContext> Engine::createCompileContext()
    {
        // we don't have any views / renderGraphs assigned to the viewer yet so assigning a ref_ptr<ResourceHints> will not be effective.
        // instead we'll pass resourceRequirements to CompileManager::add(..).
        auto compile = vsg::CompileManager::create(*mViewer, vsg::ref_ptr<vsg::ResourceHints>());
        auto ret = vsg::ref_ptr{ new vsgUtil::CompileContext };
        ret->device = getOrCreateDevice();
        ret->compileManager = compile;
        ret->onCompiled = [this](const vsg::CompileResult& r) { vsg::updateViewer(*mViewer, r); };
        if (mDeletionQueue)
            ret->onDetached = [this](vsg::ref_ptr<vsg::Object> object) { mDeletionQueue->add(object); };
        else
            ret->onDetached = [](vsg::ref_ptr<vsg::Object>){};
        return ret;
    }

    vsg::ref_ptr<Screenshot> Engine::screenshotOperation()
    {
        if (mWindow)
            return vsg::ref_ptr{ new Screenshot(mWindow) };
        else
            return vsg::ref_ptr{ new Screenshot(getOrCreateDevice(), mHeadless->colorImageView->image, mHeadless->presentLayout) };
    }

    size_t Engine::numFrames() const
    {
        if (mWindow)
            return mWindow->numFrames();
        else
            return 1;
    }

    vsg::ref_ptr<vsg::ImageView> Engine::colorImageView(size_t relativeFrameIndex)
    {
        if (mWindow)
            return mWindow->imageView(mWindow->imageIndex(relativeFrameIndex));
        else
            return mHeadless->colorImageView;
    }

    vsg::ref_ptr<vsg::ImageView> Engine::depthImageView()
    {
        if (mWindow)
            return mWindow->getOrCreateDepthImageView();
        else
            return mHeadless->depthImageView;
    }

    void Engine::updateExtents(const VkExtent2D& extent)
    {
        if (mWindow)
        {
            if (auto adapter = mWindow->cast<vsg::WindowAdapter>())
                adapter->updateExtents(extent.width, extent.height);
        }
        else
        {
            auto old_framebuffer = mHeadless->mRenderGraph->framebuffer;
            mHeadless->updateExtents(extent);

            for (auto& task : mViewer->recordAndSubmitTasks)
            {
                for (auto& cg : task->commandGraphs)
                {
                    if (cg->framebuffer == old_framebuffer)
                        cg->framebuffer = mHeadless->mRenderGraph->framebuffer;
                }
            }
        }
    }

    const VkPhysicalDeviceFeatures& Engine::getEnabledFeatures() const
    {
        return mTraits->deviceFeatures->get();
    }

    VkSampleCountFlagBits Engine::framebufferSamples()
    {
        return supportedSamples(mTraits->samples, getOrCreatePhysicalDevice());
    }

    void Engine::setRenderPass(vsg::ref_ptr<vsg::RenderPass> renderPass)
    {
        if (mWindow)
            mWindow->setRenderPass(renderPass);
        else
            mHeadless->setRenderPass(renderPass);
    }

    VkImageLayout Engine::getPresentLayout() const
    {
        // we can't use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR unless the associated extension has been enabled in vsg::Device creation.
        if (mWindow)
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        else
            return mHeadless->presentLayout;
    }
}
