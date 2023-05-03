// vsgopenmw-unity-build

#include <stdexcept>

#include <vsg/app/RenderGraph.h>
#include <vsg/app/View.h>
#include <vsg/commands/Commands.h>

#include "attachmentformat.hpp"
#include "download.hpp"

namespace Render // namespace vsgExamples
{
    /*
     * Allows rendering to virtual displays.
     */
    class Headless
    {
        int mQueueFamily = 0;
        VkExtent2D mExtent;
        vsg::ref_ptr<vsg::Image> mHostImage;
        vsg::ref_ptr<vsg::Device> mDevice;
        vsg::ref_ptr<vsg::Data> mDownloadData;
        vsg::ref_ptr<vsg::WindowTraits> mTraits;

        vsg::ref_ptr<vsg::ImageView> createImageView(
            vsg::ref_ptr<vsg::Device> device, const VkExtent2D& extent, VkFormat imageFormat, uint32_t usage)
        {
            auto colorImage = vsg::Image::create();
            colorImage->imageType = VK_IMAGE_TYPE_2D;
            colorImage->format = imageFormat;
            colorImage->extent = { extent.width, extent.height, 1 };
            colorImage->mipLevels = 1;
            colorImage->arrayLayers = 1;
            colorImage->usage = usage;
            return vsg::createImageView(device, colorImage, vsg::computeAspectFlagsForFormat(imageFormat));
        }

    public:
        vsg::ref_ptr<vsg::ImageView> colorImageView;
        vsg::ref_ptr<vsg::ImageView> depthImageView;
        vsg::ref_ptr<vsg::Commands> commands;
        vsg::ref_ptr<vsg::RenderGraph> mRenderGraph;

        using Callback = std::function<void(vsg::ref_ptr<vsg::Data> pixels)>;
        Callback callback;

        Headless(Callback in_callback, vsg::ref_ptr<vsg::WindowTraits> traits)
        {
            callback = in_callback;
            mTraits = traits;
            mTraits->validate();
            mExtent.width = mTraits->width;
            mExtent.height = mTraits->height;
        }

        int getQueueFamily() const { return mQueueFamily; }
        VkExtent2D extent2D() const { return mExtent; }

        vsg::ref_ptr<vsg::RenderGraph> createRenderGraph()
        {
            mRenderGraph = vsg::RenderGraph::create();
            resizeRenderGraph();
            return mRenderGraph;
        }
        vsg::ref_ptr<vsg::Device> getOrCreateDevice()
        {
            if (mDevice)
                return mDevice;
            auto instance = vsg::Instance::create(mTraits->instanceExtensionNames, mTraits->requestedLayers);
            vsg::ref_ptr<vsg::PhysicalDevice> physicalDevice;
            std::tie(physicalDevice, mQueueFamily) = instance->getPhysicalDeviceAndQueueFamily(mTraits->queueFlags);
            if (!physicalDevice || mQueueFamily < 0)
                throw std::runtime_error("Could not create PhysicalDevice");

            vsg::QueueSettings queueSettings{ vsg::QueueSetting{ mQueueFamily, { 1.0 } } };
            mDevice = vsg::Device::create(physicalDevice, queueSettings, mTraits->requestedLayers,
                mTraits->deviceExtensionNames, mTraits->deviceFeatures);
            return mDevice;
        }

        void updateExtents(const VkExtent2D& e)
        {
            mExtent = e;
            if (mRenderGraph)
                resizeRenderGraph();
        }
        void resizeRenderGraph()
        {
            auto device = getOrCreateDevice();
            auto colorFormat = compatibleColorFormat;
            colorImageView = createImageView(device, mExtent, colorFormat, mTraits->swapchainPreferences.imageUsage);
            mDownloadData = vsg::ubvec4Array2D::create(mExtent.width, mExtent.height, vsg::Data::Properties(colorFormat));

            // create support for copying the color buffer
            vsg::ref_ptr<vsg::Commands> tmp_commands;
            std::tie(tmp_commands, mHostImage) = createDownloadCommands(
                mDevice, colorImageView->image, { { 0, 0 }, mExtent }, colorImageView->image->format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            if (commands)
                commands->children = tmp_commands->children;
            else
                commands = tmp_commands;

            auto depthFormat = /*mTraits->depthFormat*/ compatibleDepthFormat;
            depthImageView = createImageView(device, mExtent, depthFormat, mTraits->depthImageUsage);

            vsg::ref_ptr<vsg::RenderPass> renderPass(mRenderGraph->getRenderPass());
            bool requiresDepthRead = (mTraits->depthImageUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
            if (!renderPass)
                renderPass = vsg::createRenderPass(device, colorFormat, depthFormat, requiresDepthRead);
            auto framebuffer = vsg::Framebuffer::create(
                renderPass, vsg::ImageViews{ colorImageView, depthImageView }, mExtent.width, mExtent.height, 1);
            mRenderGraph->framebuffer = framebuffer;
            mRenderGraph->renderArea = { { 0, 0 }, mExtent };
            mRenderGraph->setClearValues(VkClearColorValue{ { 0.2f, 0.2f, 0.2f, 1.f } });
        }

        void copyOutput()
        {
            if (mapAndCopy(mDevice, mHostImage, mDownloadData))
                callback(mDownloadData);
        }
    };
}
