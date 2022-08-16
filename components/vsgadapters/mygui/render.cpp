#include "render.hpp"

#include <MyGUI_Timer.h>

#include <vsg/core/Array.h>
#include <vsg/vk/Context.h>
#include <vsg/vk/State.h>
#include <vsg/traversals/RecordTraversal.h>
#include <vsg/traversals/CompileTraversal.h>
#include <vsg/io/Options.h>
#include <vsg/state/ViewportState.h>
#include <vsg/state/VertexInputState.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/state/MultisampleState.h>
#include <vsg/state/DepthStencilState.h>
#include <vsg/state/BindDynamicDescriptorSet.h>
#include <vsg/state/BufferedDescriptorBuffer.h>
#include <vsg/commands/Draw.h>
#include <vsgXchange/glsl.h>

#include <components/vsgutil/readshader.hpp>

#include "texture.hpp"

namespace
{
    const int viewID = 0;

vsg::GraphicsPipelineStates createPipelineStates(VkBlendFactor srcBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, VkBlendFactor dstBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
{
    vsg::ColorBlendState::ColorBlendAttachments colorBlendAttachments {{true,
        srcBlendFactor, dstBlendFactor, VK_BLEND_OP_ADD,
        srcBlendFactor, dstBlendFactor, VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
    }};

    auto depthStencilState = vsg::DepthStencilState::create();
    depthStencilState->depthTestEnable = false;
    depthStencilState->depthWriteEnable = false;

    auto rasterState = vsg::RasterizationState::create();
    rasterState->cullMode = VK_CULL_MODE_NONE;

    /*vsg::VertexInputState::Bindings vertexBindingsDescriptions = {VkVertexInputBindingDescription{0, sizeof(MyGUI::Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    vsg::VertexInputState::Attributes vertexAttributeDescriptions = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyGUI::Vertex, x)},
        {1, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(MyGUI::Vertex, colour)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyGUI::Vertex, u)}
    };
    */

    return {
        vsg::VertexInputState::create(),//vertexBindingsDescriptions, vertexAttributeDescriptions),
        vsg::InputAssemblyState::create(),
        rasterState,
        vsg::MultisampleState::create(),
        vsg::ColorBlendState::create(colorBlendAttachments),
        depthStencilState};
}

}

namespace vsgAdapters::mygui
{

class VertexBuffer : public MyGUI::IVertexBuffer
{
public:
    vsg::PipelineLayout *mLayout{};
    size_t mNeedVertexCount = 0;
    struct Buffer
    {
        vsg::ref_ptr<vsg::Data> mArray;
        vsg::ref_ptr<vsg::DescriptorBuffer> mBuffer;
        vsg::ref_ptr<vsg::BindDescriptorSet> mBind;
        bool mCompiled = false;
    } mBuffer;
    void setVertexCount(size_t count) override
    {
        mNeedVertexCount = count;
    }
    size_t getVertexCount() const override
    {
        return mNeedVertexCount;
    }
    MyGUI::Vertex *lock() override
    {
        if (!mBuffer.mArray || mBuffer.mArray->dataSize() < mNeedVertexCount*sizeof(MyGUI::Vertex))
        {
            mBuffer.mArray = vsg::ubyteArray::create(mNeedVertexCount*sizeof(MyGUI::Vertex));
            //vsg::createHostVisibleVertexBuffer()
            mBuffer.mBuffer = vsg::BufferedDescriptorBuffer::create(mBuffer.mArray, 0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
            auto set = 1;
            auto descriptorSet = vsg::DescriptorSet::create(mLayout->setLayouts[set], vsg::Descriptors{mBuffer.mBuffer});
            mBuffer.mBind = vsg::BindDynamicDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout, set, descriptorSet);
            mBuffer.mCompiled = false;
        }
        return reinterpret_cast<MyGUI::Vertex*>(mBuffer.mArray->dataPointer());
    }
    void unlock() override
    {
        if (mBuffer.mCompiled)
            mBuffer.mBuffer->copyDataListToBuffers();
    }
};
class Node : public vsg::Node
{
public:
    Node(Render &Render) : mRender(Render) {}

    void accept(vsg::RecordTraversal &visitor) const override
    {
        mRender.render(visitor);
    }

private:
    Render &mRender;
};

Render::Render(MyGUI::IntSize extent, vsg::Context *context, const vsg::Options *options, float scalingFactor)
    : mOptions(options)
    , mContext(context)
{
    mNode = vsg::ref_ptr{new Node(*this)};

    mCompileTraversal = vsg::CompileTraversal::create(context->device);

    if (scalingFactor != 0.f)
        mInvScalingFactor = 1.f / scalingFactor;
    setViewSize(extent.width, extent.height);

    vsg::DescriptorSetLayoutBindings textureBindings = {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
    vsg::DescriptorSetLayoutBindings vertexBindings = {{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}};
    mPipelineLayout = vsg::PipelineLayout::create(
        vsg::DescriptorSetLayouts{
            vsg::DescriptorSetLayout::create(textureBindings),
            vsg::DescriptorSetLayout::create(vertexBindings)},
        vsg::PushConstantRanges{});
}

Render::~Render()
{
    mContext = nullptr;
}

MyGUI::IVertexBuffer* Render::createVertexBuffer()
{
    auto buffer = new VertexBuffer;
    buffer->mLayout = mPipelineLayout;
    return buffer;
}

void Render::destroyVertexBuffer(MyGUI::IVertexBuffer *buffer)
{
    //vsgopenmw-deletion-queue
    if (mContext)
        vkDeviceWaitIdle(mContext->device->getDevice());
    delete buffer;
}

void Render::doRender(MyGUI::IVertexBuffer *buffer, MyGUI::ITexture *texture, size_t count)
{
    auto tex = static_cast<Texture*>(texture);
    //assert(tex->mTexture.valid();
    if (!tex->mTexture.valid())
        return;

    auto vertexBuffer = static_cast<VertexBuffer*>(buffer);
    auto &b = vertexBuffer->mBuffer;
    if (!b.mCompiled || !tex->mCompiled)
    {
        if (!b.mCompiled)
            mCompileTraversal->apply(*(b.mBind));
        if (!tex->mCompiled)
        {
            if (!tex->mShader.empty())
            {
                auto idx = std::distance(mPipelineNames.begin(), std::find(mPipelineNames.begin(), mPipelineNames.end(), tex->mShader));
                tex->mPipeline = mPipelines[idx];
            }
            tex->createBindDescriptorSet(mPipelineLayout.get());
            mCompileTraversal->apply(*tex->mBindDescriptorSet);
        }
        auto ctx = mCompileTraversal->contexts.back();
        ctx->renderPass = mContext->renderPass;
        ctx->viewID = 0;
        compilePipelines();
        ctx->record();
        ctx->waitForCompletion();

        b.mCompiled = true;
        tex->mCompiled = true;
    }

    if (tex->mPipeline)
        mCurrentTraversal->apply(*tex->mPipeline);
    else
        mCurrentTraversal->apply(*mPipelines[0]);

    mCurrentTraversal->apply(*tex->mBindDescriptorSet);
    mCurrentTraversal->apply(*(b.mBind));
    vsg::Draw draw(count,1,0,0);
    mCurrentTraversal->apply(draw);
}

void Render::render(vsg::RecordTraversal &record)
{
    mCurrentTraversal = &record;
    record.getState()->_commandBuffer->viewID = viewID;
    onRenderToTarget(this, mUpdate);
    mUpdate = false;
    //record.getState()->dirty();
}

void Render::setViewSize(int width, int height)
{
    width = std::max(1,width);
    height = std::max(1,height);

    mViewSize.set(width * mInvScalingFactor, height * mInvScalingFactor);

    mInfo.maximumDepth = 1;
    mInfo.hOffset = 0;
    mInfo.vOffset = 0;
    mInfo.aspectCoef = float(mViewSize.height) / float(mViewSize.width);
    mInfo.pixScaleX = 1.0f / float(mViewSize.width);
    mInfo.pixScaleY = 1.0f / float(mViewSize.height);

    auto &ctx = mCompileTraversal->contexts.back();
    ctx->defaultPipelineStates = {vsg::ViewportState::create(0, 0, width, height)};
    if (!mPipelines.empty())
    {
        vkDeviceWaitIdle(ctx->device->getDevice());
        compilePipelines(true);
        ctx->record();
        ctx->waitForCompletion();
    }

    onResizeView(mViewSize);
    mUpdate = true;
}

MyGUI::ITexture* Render::createTexture(const std::string &name)
{
    auto item = mTextures.find(name);
    if (item != mTextures.end())
        destroyTexture(item->second.get());
    const auto it = mTextures.emplace(name, std::make_unique<Texture>(name, mOptions, mContext->copyImageCmd)).first;
    return it->second.get();
}

void Render::destroyTexture(MyGUI::ITexture *texture)
{
    //vsgopenmw-deletion-queue
    if (mContext)
        vkDeviceWaitIdle(mContext->device->getDevice());
    mTextures.erase(texture->getName());
}

MyGUI::ITexture* Render::getTexture(const std::string &name)
{
    auto item = mTextures.find(name);
    if(item == mTextures.end())
    {
        MyGUI::ITexture* tex = createTexture(name);
        tex->loadFromFile(name);
        return tex;
    }
    return item->second.get();
}

void Render::registerShader(const std::string& shaderName, const std::string& vertexProgramFile, const std::string& fragmentProgramFile)
{
    auto options = vsg::Options::create();
    options->add(vsgXchange::glsl::create());

    auto shaders = vsg::ShaderStages{vsgUtil::readShader(vertexProgramFile, options), vsgUtil::readShader(fragmentProgramFile, options)};

    auto pipeline = vsg::BindGraphicsPipeline::create(vsg::GraphicsPipeline::create(mPipelineLayout, shaders, createPipelineStates()));
    mPipelineNames.emplace_back(shaderName);
    mPipelines.emplace_back(pipeline);

    if (shaderName.empty())
    {
        mPipelineNames.emplace_back("premult_alpha");
        mPipelines.emplace_back(vsg::BindGraphicsPipeline::create(vsg::GraphicsPipeline::create(mPipelineLayout, shaders, createPipelineStates(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA))));

        mPipelineNames.emplace_back("blend_add");
        mPipelines.emplace_back(vsg::BindGraphicsPipeline::create(vsg::GraphicsPipeline::create(mPipelineLayout, shaders, createPipelineStates(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE))));
    }
}

void Render::compilePipelines(bool release)
{
    for (auto &p : mPipelines)
    {
        if (release)
            p->pipeline->release();
        p->accept(*mCompileTraversal);
    }
}

}
