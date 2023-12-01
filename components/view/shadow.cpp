#include "shadow.hpp"

#include <iostream>

#include <vsg/app/RenderGraph.h>
#include <vsg/app/View.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/vk/Context.h>

#include <components/pipeline/viewbindings.hpp>
#include <components/pipeline/viewdata.hpp>
#include <components/render/cascade.hpp>
#include <components/vsgutil/image.hpp>

namespace
{
#include <files/shaders/lib/view/cascades.glsl>

    vsg::ref_ptr<vsg::RenderPass> createDepthRenderPass(vsg::Device* device, VkFormat format)
    {
        auto attachment = vsg::defaultDepthAttachment(format);
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        vsg::RenderPass::Attachments attachments{ attachment };

        vsg::AttachmentReference depthReference = {};
        depthReference.attachment = 0;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        vsg::SubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.depthStencilAttachments = { depthReference };
        vsg::RenderPass::Subpasses subpasses = { subpass };

        // Use subpass dependencies for layout transitions
        auto dependencies = vsg::RenderPass::Dependencies{
            { VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT },
            { 0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT }
        };
        return vsg::RenderPass::create(device, attachments, subpasses, dependencies);
    }

    /*
     * Permutes existing GraphicsPipelineStates.
     */
    class OverrideState : public vsg::Inherit<vsg::GraphicsPipelineState, OverrideState>
    {
    public:
        bool depthClampEnable = false;
        VkCullModeFlags cullMode = VK_CULL_MODE_FRONT_BIT;
        void apply(vsg::Context& context, VkGraphicsPipelineCreateInfo& pipelineInfo) const override
        {
            auto& rasterState = const_cast<VkPipelineRasterizationStateCreateInfo&>(*pipelineInfo.pRasterizationState);
            rasterState.depthClampEnable = depthClampEnable;
            rasterState.cullMode = cullMode;
        }
    };
}

namespace View
{
    struct Cascade
    {
        Cascade(
            vsg::ref_ptr<vsg::RenderGraph> in_rendergraph, const vsg::View& shared_baseView)
            : rendergraph(in_rendergraph)
        {
            view = vsg::View::create(shared_baseView); // shareViewID()
            view->camera = vsg::Camera::create(ortho, lookAt,
                vsg::ViewportState::create(0, 0, in_rendergraph->renderArea.extent.width, in_rendergraph->renderArea.extent.height));
        }
        vsg::ref_ptr<vsg::RenderGraph> rendergraph;
        vsg::ref_ptr<vsg::Orthographic> ortho = vsg::Orthographic::create();
        vsg::ref_ptr<vsg::LookAt> lookAt = vsg::LookAt::create();
        vsg::ref_ptr<vsg::View> view;
    };

    Shadow::Shadow(vsg::Context& ctx, int in_numCascades, uint32_t res, bool filter, bool supportsDepthClamp, bool frontFaceCulling)
        : resolution(res)
        , numCascades(std::max(1, std::min(in_numCascades, maxCascades)))
    {
        mBaseView = vsg::View::create(); // obtainSharedViewID()
        mBaseView->viewDependentState = nullptr;
        auto overrideState = OverrideState::create();
        if(supportsDepthClamp)
            overrideState->depthClampEnable = true; // Depth clamp to avoid near plane clipping
        else
            std::cerr << "!supportsDepthClamp(components-view-shadow)" << std::endl;
        overrideState->cullMode = frontFaceCulling ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_NONE;
        mBaseView->overridePipelineStates = {
            overrideState,
            vsg::ColorBlendState::create()
        };

        const auto format = VK_FORMAT_D32_SFLOAT;

        auto image = vsgUtil::createImage(format, VkExtent2D{ res, res }, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        image->arrayLayers = numCascades;
        image->compile(ctx);

        auto imageView = vsg::ImageView::create(image, VK_IMAGE_ASPECT_DEPTH_BIT);
        imageView->viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

        auto sampler = vsg::Sampler::create();
        sampler->addressModeU = sampler->addressModeV = sampler->addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler->minFilter = filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        sampler->magFilter = filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        sampler->maxAnisotropy = 1.0f;
        sampler->minLod = 0.0f;
        sampler->maxLod = 1.0f;
        //sampler->borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler->borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK; // vsgopenmw-reverse-depth
        sampler->compareEnable = VK_TRUE;
        sampler->compareOp = VK_COMPARE_OP_GREATER; // vsgopenmw-reverse-depth

        mShadowMap = vsg::DescriptorImage::create(
            vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(), imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL),
            Pipeline::Descriptors::VIEW_SHADOW_MAP_BINDING, 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        mShadowSampler = vsg::DescriptorImage::create(
            vsg::ImageInfo::create(sampler, vsg::ref_ptr<vsg::ImageView>()),
            Pipeline::Descriptors::VIEW_SHADOW_SAMPLER_BINDING, 0, VK_DESCRIPTOR_TYPE_SAMPLER);

        auto renderPass = createDepthRenderPass(ctx.device, format);

        for (int i = 0; i < numCascades; ++i)
        {
            // Image view for this cascade's layer (inside the depth map)
            // This view is used to render to that specific depth image layer
            auto cascadeImageView = vsg::ImageView::create(image, VK_IMAGE_ASPECT_DEPTH_BIT);
            cascadeImageView->subresourceRange.layerCount = 1;
            cascadeImageView->subresourceRange.baseArrayLayer = i;
            cascadeImageView->viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            cascadeImageView->compile(ctx.device);

            auto rendergraph = vsg::RenderGraph::create();
            rendergraph->renderArea.offset = {};
            rendergraph->renderArea.extent = { res, res };
            rendergraph->framebuffer
                = vsg::Framebuffer::create(renderPass, vsg::ImageViews{ cascadeImageView }, res, res, 1);
            rendergraph->clearValues = { { .depthStencil = { Render::farDepth, 0 } } };
            mCascades.emplace_back(std::make_unique<Cascade>(rendergraph, *mBaseView));
        }
    }

    Shadow::~Shadow() {}

    vsg::Descriptor* Shadow::shadowMap()
    {
        return mShadowMap;
    }

    vsg::Descriptor* Shadow::shadowSampler()
    {
        return mShadowSampler;
    }

    vsg::ref_ptr<vsg::View> Shadow::cascadeView(int i)
    {
        return mCascades[i]->view;
    }

    vsg::ref_ptr<vsg::RenderGraph> Shadow::renderGraph(int i)
    {
        return mCascades[i]->rendergraph;
    }

    void Shadow::updateCascades(const vsg::Camera& camera, Pipeline::Data::Scene& data, const vsg::vec3& lightPos, float maxZ, float cascadeSplitLambda)
    {
        auto splits = Render::cascade(mCascades.size(), data.zNear, data.zFar, maxZ, cascadeSplitLambda, camera,
            vsg::normalize(lightPos * -1.f));
        const vsg::dmat4 clipSpaceToUv(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.5, 0.5, 0.0, 1.0);
        for (size_t i = 0; i < splits.size(); ++i)
        {
            auto& c = mCascades[i];
            auto& split = splits[i];
            *c->ortho = split.ortho;
            *c->lookAt = split.lookAt;
            data.cascadeSplits[i] = split.splitDepth;
            data.viewToCascadeProj[i] = vsg::mat4(clipSpaceToUv * split.ortho.transform() * split.lookAt.transform()
                * vsg::inverse_4x3(camera.viewMatrix->transform()));
            data.cascadeFrustumWidths[i] = split.ortho.right - split.ortho.left;
        }
    }
}
