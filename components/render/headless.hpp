// vsgopenmw-unity-build

#include <stdexcept>
#include <cassert>

#include <vsg/app/RenderGraph.h>
#include <vsg/app/View.h>
#include <vsg/commands/Commands.h>
#include <vsg/commands/PipelineBarrier.h>
#include <vsg/vk/SubmitCommands.h>

#include <components/vsgutil/image.hpp>

#include "attachmentformat.hpp"
#include "download.hpp"
#include "multisample.hpp"

namespace Render // namespace vsgExamples
{
    /*
     * Allows rendering to virtual displays.
     */
    class Headless
    {
        int mQueueFamily = -1;
        vsg::ref_ptr<vsg::Instance> mInstance;
        vsg::ref_ptr<vsg::PhysicalDevice> mPhysicalDevice;
        vsg::ref_ptr<vsg::Device> mDevice;
        vsg::ref_ptr<vsg::RenderPass> mRenderPass;
        VkExtent2D mExtent;
        vsg::ref_ptr<vsg::Image> mHostImage;
        vsg::ref_ptr<vsg::Data> mDownloadData;
        vsg::ref_ptr<vsg::WindowTraits> mTraits;

        vsg::ref_ptr<vsg::ImageView> createImageView(vsg::ref_ptr<vsg::Device> device, const VkExtent2D& extent,
            VkFormat imageFormat, VkImageUsageFlags usage, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
        {
            auto image = vsgUtil::createImage(imageFormat, extent, usage, samples);
            return vsg::createImageView(device, image, vsg::computeAspectFlagsForFormat(imageFormat));
        }

    public:
        vsg::ref_ptr<vsg::ImageView> colorImageView;
        vsg::ref_ptr<vsg::ImageView> depthImageView;
        vsg::ref_ptr<vsg::Commands> commands;
        vsg::ref_ptr<vsg::RenderGraph> mRenderGraph;
        static constexpr VkImageLayout presentLayout = VK_IMAGE_LAYOUT_GENERAL;

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

        vsg::ref_ptr<vsg::RenderPass> getRenderPass() { return mRenderPass; }
        void setRenderPass(vsg::ref_ptr<vsg::RenderPass> renderPass) {
            mRenderPass = renderPass;
            if (mRenderGraph)
                mRenderGraph->renderPass = renderPass;
        }
        vsg::ref_ptr<vsg::RenderGraph> createRenderGraph()
        {
            mRenderGraph = vsg::RenderGraph::create();
            mRenderGraph->renderPass = mRenderPass;
            resizeRenderGraph();
            return mRenderGraph;
        }
        vsg::ref_ptr<vsg::Instance> getOrCreateInstance()
        {
            if (!mInstance)
                mInstance = vsg::Instance::create(mTraits->instanceExtensionNames, mTraits->requestedLayers);
            return mInstance;
        }
        vsg::ref_ptr<vsg::PhysicalDevice> getOrCreatePhysicalDevice()
        {
            if (mPhysicalDevice)
                return mPhysicalDevice;
            std::tie(mPhysicalDevice, mQueueFamily) = getOrCreateInstance()->getPhysicalDeviceAndQueueFamily(mTraits->queueFlags);
            if (!mPhysicalDevice || mQueueFamily < 0)
                throw std::runtime_error("Could not create PhysicalDevice");
            return mPhysicalDevice;
        }
        vsg::ref_ptr<vsg::Device> getDevice()
        {
            return mDevice;
        }
        vsg::ref_ptr<vsg::Device> getOrCreateDevice()
        {
            if (mDevice)
                return mDevice;
            auto physicalDevice = getOrCreatePhysicalDevice();
            vsg::QueueSettings queueSettings{ vsg::QueueSetting{ mQueueFamily, { 1.0 } } };
            mDevice = vsg::Device::create(physicalDevice, queueSettings, mTraits->requestedLayers, mTraits->deviceExtensionNames, mTraits->deviceFeatures);
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
            // compute the sample bits to use
            auto samples = supportedSamples(mTraits->samples, mPhysicalDevice);

            auto colorFormat = compatibleColorFormat;

            auto depthFormat = /*mTraits->depthFormat*/ compatibleDepthFormat;

            bool requiresDepthRead = (mTraits->depthImageUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
            bool multisampling = samples != VK_SAMPLE_COUNT_1_BIT;
            /*
            if (!mRenderPass)
                mRenderPass = vsg::createMultisampledRenderPass(device, colorFormat, depthFormat, samples, requiresDepthRead);
            */
            assert(mRenderPass);
            vsg::ImageViews attachments;
            vsg::ref_ptr<vsg::ImageView> multisampleImageView;
            if (multisampling)
            {
                multisampleImageView = createImageView(device, mExtent, colorFormat, mTraits->swapchainPreferences.imageUsage, samples);
                attachments.push_back(multisampleImageView);
            }
            colorImageView = createImageView(device, mExtent, colorFormat, mTraits->swapchainPreferences.imageUsage);
            attachments.push_back(colorImageView);
            if (multisampling)
            {
                auto multisampleDepthImageView = createImageView(device, mExtent, depthFormat, mTraits->depthImageUsage, samples);
                attachments.push_back(multisampleDepthImageView);
            }
            if (!multisampling || (multisampling && requiresDepthRead))
            {
                depthImageView = createImageView(device, mExtent, depthFormat, mTraits->depthImageUsage);
                attachments.push_back(depthImageView);
            }
            auto framebuffer = vsg::Framebuffer::create(mRenderPass, attachments, mExtent.width, mExtent.height, 1);

            // ensure image attachments are setup on GPU.
            {
                auto commandPool = vsg::CommandPool::create(mDevice, mQueueFamily);
                vsg::submitCommandsToQueue(commandPool, mDevice->getQueue(mQueueFamily), [&](vsg::CommandBuffer& commandBuffer) {
                    auto depthImageBarrier = vsg::ImageMemoryBarrier::create(
                        0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                        depthImageView->image,
                        depthImageView->subresourceRange);

                    auto pipelineBarrier = vsg::PipelineBarrier::create(
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        0, depthImageBarrier);
                    pipelineBarrier->record(commandBuffer);

                    if (multisampling)
                    {
                        auto msImageBarrier = vsg::ImageMemoryBarrier::create(
                            0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                            multisampleImageView->image,
                            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
                        auto msPipelineBarrier = vsg::PipelineBarrier::create(
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            0, msImageBarrier);
                        msPipelineBarrier->record(commandBuffer);
                    }
                });
            }

            mRenderGraph->framebuffer = framebuffer;
            mRenderGraph->renderArea = { { 0, 0 }, mExtent };
            mRenderGraph->setClearValues(VkClearColorValue{ { 0.2f, 0.2f, 0.2f, 1.f } });

            // create support for copying the color buffer
            mDownloadData = vsg::ubvec4Array2D::create(mExtent.width, mExtent.height, vsg::Data::Properties(colorFormat));

            vsg::ref_ptr<vsg::Commands> tmp_commands;
            std::tie(tmp_commands, mHostImage) = createDownloadCommands(
                mDevice, colorImageView->image, { { 0, 0 }, mExtent }, colorImageView->image->format, presentLayout);
            if (commands)
                commands->children = tmp_commands->children;
            else
                commands = tmp_commands;
        }

        void copyOutput()
        {
            if (mapAndCopy(mDevice, mHostImage, mDownloadData))
                callback(mDownloadData);
        }
    };
}
