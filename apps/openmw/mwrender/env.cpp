#include "env.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

#include <vsg/state/BindDescriptorSet.h>

#include <components/pipeline/layout.hpp>
#include <components/pipeline/object.hpp>
#include <components/pipeline/sets.hpp>
#include <components/pipeline/viewbindings.hpp>
#include <components/vsgadapters/osgcompat.hpp>
#include <components/vsgutil/addchildren.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/view/descriptors.hpp>

#include "../mwworld/class.hpp"

namespace MWRender
{
    vsg::ref_ptr<vsg::Data> readEnvData(vsg::ref_ptr<const vsg::Options> options)
    {
        vsg::DataList images;
        for (int i = 0; i < Pipeline::Descriptors::VIEW_ENV_COUNT; ++i)
        {
            std::stringstream stream;
            stream << "magicitem/caust";
            stream << std::setw(2);
            stream << std::setfill('0');
            stream << i;
            stream << ".dds";
            images.emplace_back(vsgUtil::readImage(stream.str(), options));
        }
        /*
         * Converts textures to a texture array to circumvent resource limits.
         */
        auto width = images[0]->width();
        auto height = images[0]->height();
        auto depth = images.size();
        auto properties = images[0]->properties;
        properties.imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        properties.maxNumMipmaps = 0;
        vsg::ref_ptr<vsg::Data> array;
        if (images[0]->valueSize() == 8)
            array = vsg::block64Array3D::create(width, height, depth, properties);
        else if (properties.format == VK_FORMAT_R8G8B8A8_UNORM)
            array = vsg::ubvec4Array3D::create(width, height, depth, properties);
        if (!array || array->dataSize() != images[0]->dataSize() * depth)
        {
            std::cerr << "!readEnv" << std::endl;
            return {};
        }
        auto pDst = reinterpret_cast<uint8_t*>(array->dataPointer());
        for (size_t i = 0; i < depth; ++i)
        {
            auto& image = images[i];
            auto pSrc = reinterpret_cast<uint8_t*>(image->dataPointer());
            if (image->width() != width || image->height() != height || image->dataSize() != images[0]->dataSize())
                continue;
            std::memcpy(pDst, pSrc, image->dataSize());
            pDst += image->dataSize();
        }
        return array;
    }

    vsg::ref_ptr<vsg::Descriptor> readEnv(vsg::ref_ptr<const vsg::Options> options)
    {
        if (auto data = readEnvData(options))
        {
            auto sampler = vsg::Sampler::create();
            return vsg::DescriptorImage::create(sampler, data, Pipeline::Descriptors::VIEW_ENV_BINDING, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }
        else
            return View::dummyEnvMap();
    }

    vsg::ref_ptr<vsg::StateGroup> createEnv(const vsg::vec4& color)
    {
        auto sg = vsg::StateGroup::create();
        auto object = Pipeline::Object(0);
        object.value().envColor = color;
        auto layout = Pipeline::getCompatiblePipelineLayout();
        sg->stateCommands = { vsg::BindDescriptorSet::create(
            VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::OBJECT_SET, vsg::Descriptors{ object.descriptor() }) };
        return sg;
    }

    std::optional<vsg::vec4> getGlowColor(const MWWorld::ConstPtr& item)
    {
        if (!item.getClass().getEnchantment(item).empty())
            return toVsg(item.getClass().getEnchantmentColor(item));
        return {};
    }

    void addEnv(vsg::ref_ptr<vsg::Node>& node, std::optional<vsg::vec4> color)
    {
        if (color)
        {
            auto sg = createEnv(*color);
            vsgUtil::addChildren(*sg, *node);
            node = sg;
        }
    }

    void addEnv(vsg::ref_ptr<vsg::Node>& node, const MWWorld::ConstPtr& item)
    {
        addEnv(node, getGlowColor(item));
    }
}
