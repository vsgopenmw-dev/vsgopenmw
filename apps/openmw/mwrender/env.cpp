#include "env.hpp"

#include <sstream>
#include <iomanip>

#include <vsg/state/BindDescriptorSet.h>

#include <components/vsgutil/addchildren.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/pipeline/scenedata.hpp>
#include <components/pipeline/object.hpp>
#include <components/pipeline/layout.hpp>
#include <components/pipeline/sets.hpp>
#include <components/vsgadapters/osgcompat.hpp>

#include "../mwworld/class.hpp"

namespace MWRender
{
    vsg::ref_ptr<vsg::DescriptorImage> readEnv(vsg::ref_ptr<const vsg::Options> options)
    {
        auto sampler = vsg::Sampler::create();
        vsg::DataList images;
        for (int i=0; i<Pipeline::Descriptors::VIEW_ENV_COUNT; ++i)
        {
            std::stringstream stream;
            stream << "textures/magicitem/caust";
            stream << std::setw(2);
            stream << std::setfill('0');
            stream << i;
            stream << ".dds";
            images.emplace_back(vsgUtil::readImage(stream.str(), options));
        }
        auto width = images[0]->width();
        auto height = images[0]->height();
        auto depth = images.size();
        auto layout = images[0]->getLayout();
        layout.imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        layout.maxNumMipmaps = 0;
        auto array = vsg::block64Array3D::create(width, height, depth, layout);
        if (images[0]->valueSize() != 8)
            throw std::runtime_error("!valueSize");
        for (size_t i=0; i<depth; ++i)
        {
            auto &image = images[i];
            auto p = reinterpret_cast<vsg::block64*>(image->dataPointer());
            if (image->width() != width || image->height() != height)
                continue;
            for (size_t x=0; x<image->width(); ++x)
            {
                for (size_t y=0; y<image->height(); ++y)
                {
                    std::memcpy(&(*array)(x,y,i), p++, sizeof(vsg::block64));
                }
            }
        }
        return vsg::DescriptorImage::create(sampler, array, Pipeline::Descriptors::VIEW_ENV_BINDING, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    vsg::ref_ptr<vsg::StateGroup> createEnv(const vsg::vec4 &color)
    {
        auto sg = vsg::StateGroup::create();
        auto object = Pipeline::Object(0);
        object.value().envColor = color;
        auto layout = Pipeline::getCompatiblePipelineLayout();
        sg->stateCommands = {vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::OBJECT_SET,
                vsg::DescriptorSet::create(layout->setLayouts[Pipeline::OBJECT_SET], vsg::Descriptors{object.descriptor()}))
        };
        return sg;
    }

    std::optional<vsg::vec4> getGlowColor(const MWWorld::ConstPtr &item)
    {
        if(!item.getClass().getEnchantment(item).empty())
            return toVsg(item.getClass().getEnchantmentColor(item));
        return {};
    }

    void addEnv(vsg::ref_ptr<vsg::Node> &node, std::optional<vsg::vec4> color)
    {
        if (color)
        {
            auto sg = createEnv(*color);
            vsgUtil::addChildren(*sg, *node);
            node = sg;
        }
    }

    void addEnv(vsg::ref_ptr<vsg::Node> &node, const MWWorld::ConstPtr &item)
    {
         addEnv(node, getGlowColor(item));
    }
}
