#include "engine.hpp"

#include <iostream>

#include <vsg/core/Exception.h>
#include <vsg/viewer/RenderGraph.h>
#include <vsg/viewer/SecondaryCommandGraph.h>
#include <vsg/viewer/WindowAdapter.h>
#include <vsg/viewer/Viewer.h>
#include <vsg/commands/Commands.h>

#include "limitframerate.hpp"
#include "headless.hpp"
#include "screenshot.hpp"

namespace Render
{
    Engine::Engine()
    {
        mViewer = vsg::Viewer::create();
    }

    Engine::~Engine()
    {
    }

    void Engine::setup(std::vector<vsg::ref_ptr<vsg::CommandGraph>> graphs)
    {
        //mViewer = nullptr;
        //mViewer = vsg::Viewer::create();
        if (mWindow)
            mViewer->addWindow(mWindow);
        else
            graphs.back()->addChild(mHeadless->commands);
        mViewer->assignRecordAndSubmitTaskAndPresentation(graphs);
        mViewer->setupThreading();
    }

    void Engine::compile()
    {
        try
        {
            mViewer->compile();
        }
        catch (const vsg::Exception &e)
        {
            auto message = e.message + " VkResult " + std::to_string(e.result);
            std::cerr << message << std::endl;
            throw std::runtime_error(message);
        }
    }

    void Engine::loop(FrameCallback frameCallback, float framerateLimit)
    {
        auto limitFramerate = LimitFramerate::create(framerateLimit);
        while (mViewer->advanceToNextFrame())
        {
            auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(limitFramerate.getLastFrameDuration()).count();

            static float fpsTimer = 0;
            fpsTimer += dt;
            if (fpsTimer > 5)
            {
                std::cout << "FPS: " << 1.0/dt << std::endl;
                fpsTimer = 0;
            }

            mViewer->handleEvents();

            bool requiresRender = true;
            bool quit = false;
            do
            {
                frameCallback(dt, requiresRender, quit);
                if (!requiresRender)
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            while (!requiresRender && !quit);
            if (quit)
                break;

            //mViewer->update();
            mViewer->recordAndSubmit();

            //frameCallback(

            if (mHeadless)
            {
                uint64_t waitTimeout = 19999999999;
                mViewer->waitForFences(0, waitTimeout);
                mHeadless->copyOutput();
            }
            else
                mViewer->present();

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
            return mHeadless->extent;
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
            graph = vsg::CommandGraph::create(mHeadless->getOrCreateDevice(), mHeadless->queueFamily);
        graph->recordTraversal = vsg::RecordTraversal::create(nullptr, maxDescriptorSet+1);
        return graph;
    }

    vsg::ref_ptr<vsg::SecondaryCommandGraph> Engine::createSecondaryCommandGraph(vsg::ref_ptr<vsg::RenderGraph> inherit_framebuffer, int maxDescriptorSet)
    {
        vsg::ref_ptr<vsg::SecondaryCommandGraph> graph;
        if (mWindow)
            graph = vsg::SecondaryCommandGraph::create(mWindow);
        else
        {
            graph = vsg::SecondaryCommandGraph::create(mHeadless->getOrCreateDevice(), mHeadless->queueFamily);
            graph->framebuffer = inherit_framebuffer->framebuffer;
        }
        graph->recordTraversal = vsg::RecordTraversal::create(nullptr, maxDescriptorSet+1);
        return graph;
    }

    vsg::ref_ptr<vsg::RenderGraph> Engine::createRenderGraph()
    {
        vsg::ref_ptr<vsg::RenderGraph> renderGraph;
        if (mWindow)
            renderGraph = vsg::RenderGraph::create(mWindow);
        else
            renderGraph = mHeadless->createRenderGraph();
        /*if (useDynamicViewport)*/renderGraph->windowResizeHandler = nullptr;
        return renderGraph;
    }

    vsg::ref_ptr<vsg::CompileManager> Engine::createCompileManager()
    {
        return vsg::CompileManager::create(*mViewer, vsg::ref_ptr<vsg::ResourceHints>());
    }

    void Engine::setHeadless(Headless::Callback callback, vsg::ref_ptr<vsg::WindowTraits> traits)
    {
        mHeadless = std::make_unique<Headless>();
        mHeadless->setup(callback, traits);
    }

    vsg::ref_ptr<Screenshot> Engine::screenshotOperation()
    {
        if (mWindow)
            return vsg::ref_ptr{new Screenshot(mWindow)};
        else
            return vsg::ref_ptr{new Screenshot(getOrCreateDevice(), mHeadless->colorImageView->image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)};
    }

    void Engine::updateExtents(const VkExtent2D &extent)
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
                for(auto& cg : task->commandGraphs)
                {
                    if (cg->framebuffer == old_framebuffer)
                        cg->framebuffer = mHeadless->mRenderGraph->framebuffer;
                }
            }
        }
    }
}
