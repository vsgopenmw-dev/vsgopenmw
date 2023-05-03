#include "texture.hpp"

#include <iostream>
#include <stdexcept>

#include <vsg/io/Options.h>
#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/Sampler.h>

#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/share.hpp>

namespace
{
    vsg::ref_ptr<vsg::Sampler> sampler()
    {
        auto sampler = vsg::Sampler::create();
        sampler->addressModeU = sampler->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        return sampler;
    }
}

namespace vsgAdapters::mygui
{
    Texture::Texture(const std::string& name, vsg::ref_ptr<const vsg::Options> options)
        : mName(name)
        , mOptions(options)
    {
    }

    Texture::Texture(vsg::ImageView* imageView)
    {
        if (imageView)
        {
            mTexture = vsg::DescriptorImage::create(vsg::ImageInfo::create(vsgUtil::share<vsg::Sampler>(sampler),
                vsg::ref_ptr{ imageView }, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
            if (auto& data = imageView->image->data)
            {
                if (data->dynamic())
                    mUsage = MyGUI::TextureUsage::Dynamic;
            }
        }
    }

    Texture::~Texture() {}

    void Texture::createManual(int width, int height, MyGUI::TextureUsage usage, MyGUI::PixelFormat format)
    {
        mWidth = width;
        mHeight = height;
        mFormat = format;
        mUsage = usage;

        if (mFormat == MyGUI::PixelFormat::R8G8B8)
            mLockedImage = vsg::ubvec3Array2D::create(mWidth, mHeight, vsg::Data::Properties{ VK_FORMAT_R8G8B8_UNORM });
        else if (mFormat == MyGUI::PixelFormat::R8G8B8A8)
            mLockedImage
                = vsg::ubvec4Array2D::create(mWidth, mHeight, vsg::Data::Properties{ VK_FORMAT_R8G8B8A8_UNORM });
        else if (mFormat == MyGUI::PixelFormat::L8)
            mLockedImage = vsg::ubyteArray2D::create(mWidth, mHeight, vsg::Data::Properties{ VK_FORMAT_R8_UNORM });
        else if (mFormat == MyGUI::PixelFormat::L8A8)
            mLockedImage = vsg::ubvec2Array2D::create(mWidth, mHeight, vsg::Data::Properties{ VK_FORMAT_R8G8_UNORM });
        else
            throw std::runtime_error("Texture format not supported");

        if (usage == MyGUI::TextureUsage::Dynamic)
            mLockedImage->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;

        mTexture = vsg::DescriptorImage::create(vsgUtil::share<vsg::Sampler>(sampler), mLockedImage);

        if (mFormat == MyGUI::PixelFormat::L8)
            mTexture->imageInfoList[0]->imageView->components
                = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
        else if (mFormat == MyGUI::PixelFormat::L8A8)
            mTexture->imageInfoList[0]->imageView->components
                = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G };
    }

    void Texture::destroy()
    {
        mTexture = nullptr;
    }

    void Texture::loadFromFile(const std::string& fname)
    {
        // assert(!fname.empty());
        if (fname.empty())
            return;
        auto textureData = vsgUtil::readOptionalImage(fname, mOptions);
        if (!textureData)
        {
            std::cout << "Failed to load " << fname << std::endl;
            return;
        }
        mTexture = vsg::DescriptorImage::create(vsgUtil::share<vsg::Sampler>(sampler), textureData);
        mWidth = textureData->width();
        mHeight = textureData->height();
        mBindDescriptorSet = nullptr;
        mCompiled = false;
    }

    void* Texture::lock(MyGUI::TextureUsage usage)
    {
        return mLockedImage->dataPointer();
    }

    void Texture::unlock()
    {
        if (mLockedImage && mCompiled)
            mLockedImage->dirty();
        if (mUsage != MyGUI::TextureUsage::Dynamic)
            mLockedImage = nullptr;
    }

    void Texture::createBindDescriptorSet(vsg::PipelineLayout* layout)
    {
        mBindDescriptorSet
            = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, vsg::Descriptors{ mTexture });
    }
}
