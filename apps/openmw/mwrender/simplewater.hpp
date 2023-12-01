#ifndef VSGOPENMW_MWRENDER_SIMPLEWATER_H
#define VSGOPENMW_MWRENDER_SIMPLEWATER_H

#include <vsg/nodes/Switch.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/VertexDraw.h>
#include <vsg/state/ArrayState.h>
#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/utils/SharedObjects.h>

#include <components/animation/transform.hpp>
#include <components/fallback/fallback.hpp>
#include <components/pipeline/builder.hpp>
#include <components/pipeline/graphics.hpp>
#include <components/pipeline/sets.hpp>
#include <components/vsgutil/composite.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/share.hpp>
#include <components/vsgutil/viewrelative.hpp>

namespace MWRender
{
    class SimpleWater : public vsgUtil::Composite<vsg::Switch>
    {
        vsg::ref_ptr<Anim::Transform> mTransform;

    public:
        SimpleWater(float radius, const Pipeline::Builder& builder, vsg::ref_ptr<const vsg::Options> textureOptions)
        {
            mTransform = new Anim::Transform;
            mNode = vsg::Switch::create();
            mNode->children = {{ vsg::MASK_OFF, mTransform }};

            Pipeline::Options pipelineOptions{ .shader = { {}, Pipeline::modes({ Pipeline::Mode::DIFFUSE_MAP, Pipeline::Mode::VERTEX, Pipeline::Mode::NORMAL, Pipeline::Mode::TEXCOORD, Pipeline::Mode::COLOR }) },
                .numUvSets = 1,
                .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                .depthWrite = false,
                .blend = true,
                .cullMode = VK_CULL_MODE_NONE };

            auto pipeline = builder.graphics->getOrCreate(pipelineOptions);
            auto layout = pipeline->pipeline->layout;

            std::string textureFile
                = "water/" + std::string(Fallback::Map::getString("Water_SurfaceTexture")) + "00.dds";
            auto image = vsgUtil::readImage(textureFile, textureOptions);
            auto sampler = builder.createSampler();
            vsgUtil::share_if(textureOptions->sharedObjects, sampler);
            auto texture = vsg::DescriptorImage::create(sampler, image);

            auto bds = vsg::BindDescriptorSet::create(
                VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::TEXTURE_SET, vsg::Descriptors{ texture });
            auto sg = vsg::StateGroup::create();
            sg->stateCommands = { pipeline, bds };
            sg->prototypeArrayState = vsg::NullArrayState::create();

            auto vertices = vsg::vec3Array::create(4);
            auto uvs = vsg::vec2Array::create(4);
            for (int i = 0; i < 4; ++i)
            {
                vsg::vec2 side((i << 1) & 2, i & 2);
                vsg::vec2 sign(side.x * 2 - 1, side.y * 2 - 1);
                vertices->at(i) = { sign * radius, 0 };
                const float uvScale = (radius * 2) / 8192 * 6;
                uvs->at(i) = { side * uvScale };
            }

            auto normals = vsg::vec3Array::create(4, vsg::vec3(0, 0, 1));
            auto colors = vsg::vec4Array::create(4, vsg::vec4(1, 1, 1, Fallback::Map::getFloat("Water_Map_Alpha")));

            auto draw = vsg::VertexDraw::create();
            draw->vertexCount = 4;
            draw->instanceCount = 1;
            draw->assignArrays(vsg::DataList{ vertices, normals, colors, uvs });
            sg->children = { draw };

            auto viewRelative = vsg::ref_ptr{ new vsgUtil::ViewRelative };
            viewRelative->resetAxes = { 0, 0, 1 };
            viewRelative->children = { sg };
            mTransform->children = { viewRelative };
        }
        void setEnabled(bool enable)
        {
            mNode->setAllChildren(enable);
        }
        void setHeight(float h) { mTransform->translation.z = h; }
    };
}

#endif
