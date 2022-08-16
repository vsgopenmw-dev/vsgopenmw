#include "shadow.hpp"

#include <vsg/vk/Context.h>
#include <vsg/viewer/RenderGraph.h>
#include <vsg/viewer/View.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/RasterizationState.h>

#include <components/pipeline/scenedata.hpp>
#include <components/render/cascade.hpp>
#include <components/view/defaultstate.hpp>
#include <components/view/descriptors.hpp>

namespace
{
#include <files/shaders/lib/view/cascades.glsl>

    vsg::ref_ptr<vsg::RenderPass> createDepthRenderPass(vsg::Device *device, VkFormat format)
    {
        auto attachment = vsg::defaultDepthAttachment(format);
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        vsg::RenderPass::Attachments attachments{attachment};

        vsg::AttachmentReference depthReference = {};
        depthReference.attachment = 0;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        vsg::SubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.depthStencilAttachments = {depthReference};
        vsg::RenderPass::Subpasses subpasses = {subpass};

        // Use subpass dependencies for layout transitions
        auto dependencies = vsg::RenderPass::Dependencies{
        {
            VK_SUBPASS_EXTERNAL,
            0,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
        },
        {
            0,
            VK_SUBPASS_EXTERNAL,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
        }};
        return vsg::RenderPass::create(device, attachments, subpasses, dependencies);
    }
}

namespace View
{
    struct Cascade
    {
        Cascade(vsg::ref_ptr<vsg::RenderGraph> r, const vsg::View &shared_baseView, View::Descriptors &shared_descriptors)
            : rendergraph(r)
        {
            view = vsg::View::create(shared_baseView); //shareViewID()
            view->camera = vsg::Camera::create(ortho, lookAt, vsg::ViewportState::create(0, 0, r->renderArea.extent.width, r->renderArea.extent.height));

            sceneGroup = View::createDefaultState(shared_descriptors.getDescriptorSet());
            view->children = {sceneGroup};
        }
        vsg::ref_ptr<vsg::RenderGraph> rendergraph;
        vsg::ref_ptr<vsg::Orthographic> ortho = vsg::Orthographic::create();
        vsg::ref_ptr<vsg::LookAt> lookAt = vsg::LookAt::create();
        vsg::ref_ptr<vsg::View> view;
        vsg::ref_ptr<vsg::Group> sceneGroup;
    };

    Shadow::Shadow(vsg::Context &ctx, int in_numCascades, uint32_t res)
        : resolution(res)
        , numCascades(std::max(1, std::min(in_numCascades, maxCascades)))
    {
        mBaseView = vsg::View::create(); //obtainSharedViewID()
        mBaseView->viewDependentState = nullptr;
        auto rasterizationState = vsg::RasterizationState::create();
        rasterizationState->cullMode = VK_CULL_MODE_NONE;
        //if(supported)rasterizationState->depthClampEnable = true;
        mBaseView->overridePipelineStates = {rasterizationState, vsg::ColorBlendState::create()};

        mDescriptors = std::make_unique<View::Descriptors>(0);
        mDescriptors->sceneData().cascadeSplits[0] = 1; //depthPass=true;

        const auto format = VK_FORMAT_D32_SFLOAT;

        auto image = vsg::Image::create();
        image->extent = {res,res,1};
        image->mipLevels = 1;
        image->arrayLayers = numCascades;
        image->format = format;
        image->usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image->compile(ctx);

        auto imageView = vsg::ImageView::create(image, VK_IMAGE_ASPECT_DEPTH_BIT);
        imageView->viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

        auto sampler = vsg::Sampler::create();
        sampler->addressModeU = sampler->addressModeV = sampler->addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler->minFilter = VK_FILTER_NEAREST;
        sampler->magFilter = VK_FILTER_NEAREST;
        sampler->maxAnisotropy = 1.0f;
        sampler->minLod = 0.0f;
        sampler->maxLod = 1.0f;
        sampler->borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        mShadowMap = vsg::DescriptorImage::create(vsg::ImageInfo::create(sampler, imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL), Pipeline::Descriptors::VIEW_SHADOW_MAP_BINDING);

        auto renderPass = createDepthRenderPass(ctx.device, format);

        for (int i=0; i<numCascades; ++i)
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
            rendergraph->renderArea.extent = {res,res};
            rendergraph->framebuffer = vsg::Framebuffer::create(renderPass, vsg::ImageViews{cascadeImageView}, res, res, 1);
            rendergraph->clearValues = {{.depthStencil={Render::farDepth,0}}};
            mCascades.emplace_back(std::make_unique<Cascade>(rendergraph, *mBaseView, *mDescriptors));
        }
    }

    Shadow::~Shadow()
    {
    }

    vsg::Descriptor *Shadow::shadowMap()
    {
        return mShadowMap;
    }

    vsg::ref_ptr<vsg::View> Shadow::cascadeView(int i,vsg::Mask mask)
    {
        auto &c = mCascades[i];
        c->view->mask = mask;
        return c->view;
    }

    vsg::ref_ptr<vsg::RenderGraph> Shadow::renderGraph(int i)
    {
        return mCascades[i]->rendergraph;
    }

    void Shadow::updateCascades(const vsg::Camera &camera, Pipeline::Scene &data, const vsg::vec3 &lightPos, float far)
    {
        auto splits = Render::cascade(mCascades.size(), data.zNear, /*std::min(data.zFar, far)*/data.zFar, camera, vsg::normalize(lightPos * -1.f));
        const vsg::dmat4 biasMat(
            0.5, 0.0, 0.0, 0.0,
            0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.5, 0.5, 0.0, 1.0
        );
        for (size_t i=0; i<splits.size(); ++i)
        {
            auto &c = mCascades[i];
            auto &split = splits[i];
            *c->ortho = split.ortho;
            *c->lookAt = split.lookAt;
            data.cascadeSplits[i] = split.splitDepth;
            data.viewToCascadeProj[i] = vsg::mat4(biasMat * split.ortho.transform() * split.lookAt.transform() * vsg::inverse_4x3(camera.viewMatrix->transform()));
        }
        if (splits.size() < maxCascades)
            data.cascadeSplits[maxCascades-1] = -far;
    }

    void Shadow::setCastShadowScene(vsg::ref_ptr<vsg::Node> scene)
    {
        for (auto &c : mCascades)
            c->sceneGroup->children = {scene};
    }
}
