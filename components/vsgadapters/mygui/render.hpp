#ifndef VSGOPENMW_VSGADAPTERS_MYGUI_RENDER_H
#define VSGOPENMW_VSGADAPTERS_MYGUI_RENDER_H

#include <MyGUI_RenderManager.h>
#include <vsg/core/ref_ptr.h>

#include <components/vsgutil/compilecontext.hpp>
#include <components/vsgutil/composite.hpp>

namespace vsg
{
    class BufferInfo;
    class ImageInfo;
    class Context;
    class CompileTraversal;
    class PipelineLayout;
    class Context;
}
namespace vsgUtil
{
    class SetViewportState;
}

namespace vsgAdapters::mygui
{
    class Texture;

    class Render : public MyGUI::RenderManager, public MyGUI::IRenderTarget, public vsgUtil::Composite<vsg::Node>
    {
        friend class Node;
        void compilePipelines(vsg::Context& ctx);
        void render(vsg::RecordTraversal&);

        vsg::ref_ptr<vsgUtil::SetViewportState> mSetViewport;
        MyGUI::IntSize mViewSize;
        bool mUpdate = false;
        MyGUI::RenderTargetInfo mInfo;
        using MapTexture = std::map<std::string, std::unique_ptr<Texture>>;
        MapTexture mTextures;

        float mInvScalingFactor = 1.f;
        vsg::ref_ptr<const vsg::Options> mImageOptions;
        vsg::ref_ptr<const vsg::Options> mShaderOptions;
        vsg::RecordTraversal* mCurrentTraversal = nullptr;
        vsg::ref_ptr<vsg::CompileTraversal> mCompileTraversal;
        vsg::CompileTraversal* mPendingCompile{};
        std::vector<vsg::ref_ptr<vsg::BufferInfo>> mNewDynamicBufferInfo;
        std::vector<vsg::ref_ptr<vsg::ImageInfo>> mNewDynamicImageInfo;
        vsg::ref_ptr<vsgUtil::CompileContext> mCompileContext;
        vsg::ref_ptr<vsg::PipelineLayout> mPipelineLayout;

        std::vector<std::string> mPipelineNames;
        std::vector<vsg::ref_ptr<vsg::BindGraphicsPipeline>> mPipelines;

    public:
        Render(MyGUI::IntSize size, vsg::ref_ptr<vsgUtil::CompileContext> compileContext,
            const vsg::Options* imageOoptions, const vsg::Options* shaderOptions, float scalingFactor);
        ~Render();

        void setScalingFactor(float factor);

        const MyGUI::IntSize& getViewSize() const override { return mViewSize; }
        MyGUI::VertexColourType getVertexFormat() const override { return MyGUI::VertexColourType::ColourABGR; }
        MyGUI::IVertexBuffer* createVertexBuffer() override;
        void destroyVertexBuffer(MyGUI::IVertexBuffer* buffer) override;
        MyGUI::ITexture* createTexture(const std::string& name) override;
        void destroyTexture(MyGUI::ITexture* _texture) override;
        MyGUI::ITexture* getTexture(const std::string& name) override;

        void update(float dt) { onFrameEvent(dt); }

        void begin() override {}
        void end() override {}
        void doRender(MyGUI::IVertexBuffer* buffer, MyGUI::ITexture* texture, size_t count) override;

        const MyGUI::RenderTargetInfo& getInfo() const override { return mInfo; }

        void setViewSize(int width, int height) override;

        void registerShader(const std::string& _shaderName, const std::string& _vertexProgramFile,
            const std::string& _fragmentProgramFile) override;
    };

}

#endif
