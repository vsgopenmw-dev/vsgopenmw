#include "render.hpp"

#include <vsg/app/CompileManager.h>
#include <vsg/app/CompileTraversal.h>
#include <vsg/app/RecordTraversal.h>
#include <vsg/core/Array.h>
#include <vsg/io/Options.h>
#include <vsg/nodes/VertexDraw.h>
#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/DepthStencilState.h>
#include <vsg/state/DescriptorImage.h> //vsgopenmw-fixme
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/MultisampleState.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/state/VertexInputState.h>
#include <vsg/state/ViewportState.h>
#include <vsg/state/DynamicState.h>
#include <vsg/vk/CommandBuffer.h>

#include <components/vsgutil/readshader.hpp>
#include <components/vsgutil/setviewportstate.hpp>

#include "texture.hpp"

namespace
{
    const int viewID = 0;

    vsg::GraphicsPipelineStates createPipelineStates(VkBlendFactor srcBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        VkBlendFactor dstBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
    {
        vsg::ColorBlendState::ColorBlendAttachments colorBlendAttachments{ { true, srcBlendFactor, dstBlendFactor,
            VK_BLEND_OP_ADD, srcBlendFactor, dstBlendFactor, VK_BLEND_OP_ADD,
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                | VK_COLOR_COMPONENT_A_BIT } };

        auto depthStencilState = vsg::DepthStencilState::create();
        depthStencilState->depthTestEnable = false;
        depthStencilState->depthWriteEnable = false;

        auto rasterState = vsg::RasterizationState::create();
        rasterState->cullMode = VK_CULL_MODE_NONE;

        vsg::VertexInputState::Bindings vertexBindingsDescriptions
            = { VkVertexInputBindingDescription{ 0, sizeof(MyGUI::Vertex), VK_VERTEX_INPUT_RATE_VERTEX } };
        vsg::VertexInputState::Attributes vertexAttributeDescriptions
            = { { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyGUI::Vertex, x) },
                  { 1, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(MyGUI::Vertex, colour) },
                  { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyGUI::Vertex, u) } };

        return { vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
            vsg::InputAssemblyState::create(), rasterState, vsg::MultisampleState::create(),
            vsg::ColorBlendState::create(colorBlendAttachments), depthStencilState,
            vsg::DynamicState::create(VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR) };
    }

}

namespace vsgAdapters::mygui
{

    class VertexBuffer : public MyGUI::IVertexBuffer
    {
        const vsgUtil::CompileContext& mCompileContext;
    public:
        VertexBuffer(const vsgUtil::CompileContext& compile) : mCompileContext(compile) {}
        ~VertexBuffer()
        {
            if (mCompiled)
                mCompileContext.detach(mDraw);
        }
        size_t mNeedVertexCount = 0;
        vsg::ref_ptr<vsg::Data> mArray;
        vsg::ref_ptr<vsg::VertexDraw> mDraw;
        bool mCompiled = false;
        void setVertexCount(size_t count) override { mNeedVertexCount = count; }
        size_t getVertexCount() const override { return mNeedVertexCount; }
        MyGUI::Vertex* lock() override
        {
            if (!mArray || mArray->dataSize() < mNeedVertexCount * sizeof(MyGUI::Vertex))
            {
                if (mCompiled)
                    mCompileContext.detach(mDraw);
                mArray = vsg::ubyteArray::create(mNeedVertexCount * sizeof(MyGUI::Vertex));
                mArray->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA_TRANSFER_AFTER_RECORD;
                mDraw = vsg::VertexDraw::create();
                mDraw->assignArrays(vsg::DataList{ mArray });
                mDraw->instanceCount = 1;
                mCompiled = false;
            }
            return reinterpret_cast<MyGUI::Vertex*>(mArray->dataPointer());
        }
        void unlock() override
        {
            if (mCompiled)
                mArray->dirty();
        }
    };
    class Node : public vsg::Compilable
    {
    public:
        Node(Render& Render)
            : mRender(Render)
        {
        }

        void compile(vsg::Context& ctx) override { mRender.compilePipelines(ctx); }
        void accept(vsg::RecordTraversal& visitor) const override { mRender.render(visitor); }

    private:
        Render& mRender;
    };

    Render::Render(MyGUI::IntSize extent, vsg::ref_ptr<vsgUtil::CompileContext> compileContext,
        const vsg::Options* imageOptions, const vsg::Options* shaderOptions, float scalingFactor)
        : mImageOptions(imageOptions)
        , mShaderOptions(shaderOptions)
        , mCompileContext(compileContext)
    {
        mNode = vsg::ref_ptr{ new Node(*this) };

        mCompileTraversal = vsg::CompileTraversal::create(vsg::ref_ptr{mCompileContext->device});
        if (scalingFactor != 0.f)
            mInvScalingFactor = 1.f / scalingFactor;
        setViewSize(extent.width, extent.height);

        vsg::DescriptorSetLayoutBindings textureBindings
            = { { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr } };
        mPipelineLayout = vsg::PipelineLayout::create(
            vsg::DescriptorSetLayouts{ vsg::DescriptorSetLayout::create(textureBindings) }, vsg::PushConstantRanges{});
    }

    Render::~Render() { }

    MyGUI::IVertexBuffer* Render::createVertexBuffer()
    {
        auto buffer = new VertexBuffer(*mCompileContext);
        return buffer;
    }

    void Render::destroyVertexBuffer(MyGUI::IVertexBuffer* buffer)
    {
        delete buffer;
    }

    void Render::doRender(MyGUI::IVertexBuffer* buffer, MyGUI::ITexture* texture, size_t count)
    {
        auto tex = static_cast<Texture*>(texture);
        // assert(tex->mTexture.valid();
        if (!tex->mTexture.valid())
            return;

        auto vertexBuffer = static_cast<VertexBuffer*>(buffer);
        if (!vertexBuffer->mCompiled)
        {
            mCompileTraversal->apply(*(vertexBuffer->mDraw));
            mNewDynamicBufferInfo.push_back(vertexBuffer->mDraw->arrays[0]);
            vertexBuffer->mCompiled = true;
            mPendingCompile = mCompileTraversal;
        }
        if (!tex->mCompiled)
        {
            if (!tex->mShader.empty())
            {
                auto idx = std::distance(
                    mPipelineNames.begin(), std::find(mPipelineNames.begin(), mPipelineNames.end(), tex->mShader));
                tex->mPipeline = mPipelines[idx];
            }
            tex->createBindDescriptorSet(mPipelineLayout.get());
            mCompileTraversal->apply(*tex->mBindDescriptorSet);
            if (tex->getUsage() == MyGUI::TextureUsage::Dynamic)
                mNewDynamicImageInfo.push_back(tex->mTexture->imageInfoList[0]);
            tex->mCompiled = true;
            mPendingCompile = mCompileTraversal;
        }

        if (tex->mPipeline)
            mCurrentTraversal->apply(*tex->mPipeline);
        else
            mCurrentTraversal->apply(*mPipelines[0]);

        mCurrentTraversal->apply(*tex->mBindDescriptorSet);
        vertexBuffer->mDraw->vertexCount = count;
        mCurrentTraversal->apply(*(vertexBuffer->mDraw));
    }

    void Render::render(vsg::RecordTraversal& record)
    {
        mCurrentTraversal = &record;
        record.apply(*mSetViewport);
        record.getCommandBuffer()->viewID = viewID;
        onRenderToTarget(this, mUpdate);
        mUpdate = false;
        // record.getState()->dirty();
        if (mPendingCompile)
        {
            auto ctx = mCompileTraversal->contexts.back();
            ctx->record();
            ctx->waitForCompletion();

            vsg::CompileResult res;
            res.lateDynamicData.bufferInfos.swap(mNewDynamicBufferInfo);
            res.earlyDynamicData.imageInfos.swap(mNewDynamicImageInfo);
            mCompileContext->onCompiled(res);

            mPendingCompile = {};
        }
    }

    void Render::setViewSize(int width, int height)
    {
        width = std::max(1, width);
        height = std::max(1, height);

        if (!mSetViewport)
            mSetViewport = vsgUtil::SetViewportState::create(vsg::ViewportState::create());
        mSetViewport->viewportState->set(0, 0, width, height);

        mViewSize.set(width * mInvScalingFactor, height * mInvScalingFactor);

        mInfo.maximumDepth = 1;
        mInfo.hOffset = 0;
        mInfo.vOffset = 0;
        mInfo.aspectCoef = float(mViewSize.height) / float(mViewSize.width);
        mInfo.pixScaleX = 1.0f / float(mViewSize.width);
        mInfo.pixScaleY = 1.0f / float(mViewSize.height);

        onResizeView(mViewSize);
        mUpdate = true;
    }

    MyGUI::ITexture* Render::createTexture(const std::string& name)
    {
        auto item = mTextures.find(name);
        if (item != mTextures.end())
            destroyTexture(item->second.get());
        const auto it = mTextures.emplace(name, std::make_unique<Texture>(name, mImageOptions)).first;
        return it->second.get();
    }

    void Render::destroyTexture(MyGUI::ITexture* texture)
    {
        auto tex = static_cast<Texture*>(texture);
        if (tex->mCompiled)
            mCompileContext->detach(tex->mBindDescriptorSet);

        mTextures.erase(texture->getName());
    }

    MyGUI::ITexture* Render::getTexture(const std::string& name)
    {
        auto item = mTextures.find(name);
        if (item == mTextures.end())
        {
            MyGUI::ITexture* tex = createTexture(name);
            tex->loadFromFile(name);
            return tex;
        }
        return item->second.get();
    }

    void Render::registerShader(
        const std::string& shaderName, const std::string& vertexProgramFile, const std::string& fragmentProgramFile)
    {
        auto shaders = vsg::ShaderStages{ vsgUtil::readShader(vertexProgramFile, mShaderOptions),
            vsgUtil::readShader(fragmentProgramFile, mShaderOptions) };

        auto pipeline = vsg::BindGraphicsPipeline::create(
            vsg::GraphicsPipeline::create(mPipelineLayout, shaders, createPipelineStates()));
        mPipelineNames.emplace_back(shaderName);
        mPipelines.emplace_back(pipeline);

        if (shaderName.empty())
        {
            mPipelineNames.emplace_back("premult_alpha");
            mPipelines.emplace_back(vsg::BindGraphicsPipeline::create(vsg::GraphicsPipeline::create(mPipelineLayout,
                shaders, createPipelineStates(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA))));

            mPipelineNames.emplace_back("blend_add");
            mPipelines.emplace_back(vsg::BindGraphicsPipeline::create(vsg::GraphicsPipeline::create(
                mPipelineLayout, shaders, createPipelineStates(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE))));
        }
    }

    void Render::compilePipelines(vsg::Context& ctx)
    {
        ctx.viewID = viewID;
        for (auto& p : mPipelines)
            p->compile(ctx);
    }

}
