#include "water.hpp"

#include <vsg/commands/Draw.h>
#include <vsg/io/Options.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/utils/SharedObjects.h>

#include <components/animation/transform.hpp>
#include <components/pipeline/bindings.hpp>
#include <components/pipeline/builder.hpp>
#include <components/pipeline/graphics.hpp>
#include <components/pipeline/sets.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/share.hpp>

namespace MWRender
{

    Water::Water(Resource::ResourceSystem* resourceSystem)
    {
        mTransform = new Anim::Transform;
        mNode = mTransform;

        Pipeline::Options pipelineOptions{ .shader = { "water", { Pipeline::modes({ Pipeline::Mode::BUMP_MAP }) }},
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .depthWrite = false,
            .cullMode = VK_CULL_MODE_NONE };

        auto pipeline = resourceSystem->builder->graphics->getOrCreate(pipelineOptions);
        auto layout = pipeline->pipeline->layout;

        auto image = vsgUtil::readImage("water_nm.png", resourceSystem->textureOptions);
        auto sampler = resourceSystem->builder->createSampler();
        vsgUtil::share_if(resourceSystem->textureOptions->sharedObjects, sampler);
        auto bumpMap = vsg::DescriptorImage::create(sampler, image, Pipeline::Descriptors::BUMP_UNIT);
        auto bds = vsg::BindDescriptorSet::create(
            VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::TEXTURE_SET, vsg::Descriptors{ bumpMap });

        auto sg = vsg::StateGroup::create();
        sg->stateCommands = { pipeline, bds };
        sg->children = { vsg::Draw::create(4, 1, 0, 0) };

        mTransform->addChild(sg);
    }

    Water::~Water() {}

    void Water::setHeight(float h)
    {
        mTransform->translation.z = h;
    }

    bool Water::isUnderwater(const vsg::vec3& pos) const
    {
        return pos.z < mTransform->translation.z;
    }
}
