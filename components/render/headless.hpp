//vsgopenmw-unity-build

#include <stdexcept>

#include <vsg/viewer/RenderGraph.h>
#include <vsg/commands/Commands.h>
#include <vsg/viewer/View.h>
#include <vsg/commands/Commands.h>

#include "download.hpp"
#include "attachmentformat.hpp"

namespace Render //namespace vsgExamples
{
/*
 * Allows rendering to virtual displays.
 */
class Headless
{
    vsg::ref_ptr<vsg::Image> mHostImage;
    vsg::ref_ptr<vsg::Device> mDevice;
    vsg::ref_ptr<vsg::Data> mDownloadData;
    vsg::ref_ptr<vsg::WindowTraits> mTraits;

    vsg::ref_ptr<vsg::ImageView> createImageView(vsg::ref_ptr<vsg::Device> device, const VkExtent2D& extent, VkFormat imageFormat, uint32_t usage)
    {
        auto colorImage = vsg::Image::create();
        colorImage->imageType = VK_IMAGE_TYPE_2D;
        colorImage->format = imageFormat;
        colorImage->extent = {extent.width, extent.height, 1};
        colorImage->mipLevels = 1;
        colorImage->arrayLayers = 1;
        colorImage->usage = usage;
        return vsg::createImageView(device, colorImage, vsg::computeAspectFlagsForFormat(imageFormat));
    }
public:
    int queueFamily = 0;
    VkExtent2D extent;
    vsg::ref_ptr<vsg::ImageView> colorImageView;
    vsg::ref_ptr<vsg::Commands> commands;
    vsg::ref_ptr<vsg::RenderGraph> mRenderGraph;

    vsg::ref_ptr<vsg::RenderGraph> createRenderGraph()
    {
        mRenderGraph = vsg::RenderGraph::create();
        resizeRenderGraph();
        return mRenderGraph;
    }
    vsg::ref_ptr<vsg::Device> getOrCreateDevice()
    {
        if (mDevice) return mDevice;
        auto instance = vsg::Instance::create(mTraits->instanceExtensionNames, mTraits->requestedLayers);
        vsg::ref_ptr<vsg::PhysicalDevice> physicalDevice;
        std::tie(physicalDevice, queueFamily) = instance->getPhysicalDeviceAndQueueFamily(mTraits->queueFlags);
        if (!physicalDevice || queueFamily < 0)
            throw std::runtime_error("Could not create PhysicalDevice");

        vsg::QueueSettings queueSettings{vsg::QueueSetting{queueFamily, {1.0}}};
        mDevice = vsg::Device::create(physicalDevice, queueSettings, mTraits->requestedLayers, mTraits->deviceExtensionNames, mTraits->deviceFeatures);
        return mDevice;
    }

    void updateExtents(const VkExtent2D &e)
    {
        extent = e;
        if (mRenderGraph)
            resizeRenderGraph();
    }
    void resizeRenderGraph()
    {
        auto device = getOrCreateDevice();
        auto colorFormat = compatibleColorFormat;
        colorImageView = createImageView(device, extent, colorFormat, mTraits->swapchainPreferences.imageUsage);
        mDownloadData = vsg::ubvec4Array2D::create(extent.width, extent.height, vsg::Data::Layout{.format=VK_FORMAT_R8G8B8A8_UNORM});

        // create support for copying the color buffer
        vsg::ref_ptr<vsg::Commands> tmp_commands;
        std::tie(tmp_commands, mHostImage) = createDownloadCommands(mDevice, colorImageView->image, extent, colorImageView->image->format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        if (commands)
            commands->children = tmp_commands->children;
        else
            commands = tmp_commands;

        auto depthFormat = /*mTraits->depthFormat*/compatibleDepthFormat;
        auto depthImageView = createImageView(device, extent, depthFormat, mTraits->depthImageUsage);

        vsg::ref_ptr<vsg::RenderPass> renderPass (mRenderGraph ->getRenderPass());
        if (!renderPass)
            renderPass = vsg::createRenderPass(device, colorFormat, depthFormat);
        auto framebuffer = vsg::Framebuffer::create(renderPass, vsg::ImageViews{colorImageView, depthImageView}, extent.width, extent.height, 1);
        mRenderGraph->framebuffer = framebuffer;
        mRenderGraph->renderArea = {{0, 0}, extent};
        mRenderGraph->setClearValues(VkClearColorValue{{0.2f, 0.2f, 0.2f, 1.f}});
    }

    using Callback = std::function<void(vsg::ref_ptr<vsg::Data> pixels)>;
    Callback callback;
    void setup(Callback in_callback, vsg::ref_ptr<vsg::WindowTraits> traits)
    {
        callback = in_callback;
        mTraits = traits;
        mTraits->validate();
        extent.width = mTraits->width;
        extent.height = mTraits->height;
    }
    void copyOutput()
    {
        mapAndCopy(mDevice, mHostImage, mDownloadData);
        callback(mDownloadData);
    }
};
}
