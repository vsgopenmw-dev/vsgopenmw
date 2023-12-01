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
#include <components/vsgutil/arraytexture.hpp>
#include <components/vsgutil/name.hpp>
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
            auto name = stream.str();
            auto data = vsgUtil::readImage(name, options);
            vsgUtil::setName(*data, name);
            images.push_back(data);
        }
        return vsgUtil::convertToArrayTexture(images);
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
