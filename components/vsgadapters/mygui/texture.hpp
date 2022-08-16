#ifndef VSGOPENMW_VSGADAPTERS_MYGUI_TEXTURE_H
#define VSGOPENMW_VSGADAPTERS_MYGUI_TEXTURE_H

#include <MyGUI_ITexture.h>

#include <vsg/core/ref_ptr.h>

namespace vsg
{
    class Data;
    class DescriptorImage;
    class ImageView;
    class BindDescriptorSet;
    class BindGraphicsPipeline;
    class CopyAndReleaseImage;
    class PipelineLayout;
    class Options;
}

namespace vsgAdapters
{
namespace mygui
{
    class Texture : public MyGUI::ITexture {
        std::string mName;
        vsg::ref_ptr<vsg::Data> mLockedImage;
        vsg::ref_ptr<const vsg::Options> mOptions;
        vsg::CopyAndReleaseImage *mCopyImageCmd = nullptr;
        MyGUI::PixelFormat mFormat = MyGUI::PixelFormat::Unknow;
        MyGUI::TextureUsage mUsage = MyGUI::TextureUsage::Static;
        int mWidth = 0;
        int mHeight = 0;

    public:
        Texture(const std::string &name, vsg::ref_ptr<const vsg::Options> options, vsg::CopyAndReleaseImage *copyImageCmd = nullptr);
        Texture(vsg::ImageView *image={});
        virtual ~Texture();

        vsg::BindGraphicsPipeline *mPipeline = nullptr;
        vsg::ref_ptr<vsg::DescriptorImage> mTexture;
        vsg::ref_ptr<vsg::BindDescriptorSet> mBindDescriptorSet;
        void createBindDescriptorSet(vsg::PipelineLayout *layout);
        void update(vsg::CopyAndReleaseImage &cmd);

        bool mCompiled = false;
        std::string mShader;

        const std::string& getName() const override { return mName; }

        void createManual(int width, int height, MyGUI::TextureUsage usage, MyGUI::PixelFormat format) override;
        void loadFromFile(const std::string &fname) override;
        void saveToFile(const std::string &fname) override {}

        void destroy() override;

        void *lock(MyGUI::TextureUsage access) override;
        void unlock() override;
        bool isLocked() const override { return mLockedImage.valid(); }

        int getWidth() const override { return mWidth; }
        int getHeight() const override { return mHeight; }

        MyGUI::PixelFormat getFormat() const override { return mFormat; }
        MyGUI::TextureUsage getUsage() const override { return mUsage; }
        size_t getNumElemBytes() const override { return mFormat.getBytesPerPixel(); }

        MyGUI::IRenderTarget *getRenderTarget() override { return nullptr; }
        void setShader(const std::string& shader) override { mShader = shader; }
    };
}
}

#endif
