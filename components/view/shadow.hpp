#ifndef VSGOPENMW_VIEW_SHADOW_H
#define VSGOPENMW_VIEW_SHADOW_H

#include <memory>
#include <vector>

#include <vsg/nodes/Node.h>

namespace Pipeline::Data
{
    class Scene;
}
namespace vsg
{
    class Context;
}
namespace View // namespace vulkanexamples
{
    class Cascade;

    /*
     * Renders shadow map.
     */
    class Shadow
    {
        std::vector<std::unique_ptr<Cascade>> mCascades;
        vsg::ref_ptr<vsg::Descriptor> mShadowMap;
        vsg::ref_ptr<vsg::Descriptor> mShadowSampler;
        vsg::ref_ptr<vsg::View> mBaseView;

    public:
        Shadow(vsg::Context& ctx, int in_numCascades, uint32_t res, bool filter, bool supportsDepthClamp, bool frontFaceCulling);
        ~Shadow();
        const uint32_t resolution;
        const int numCascades;

        vsg::Descriptor* shadowMap();
        vsg::Descriptor* shadowSampler();

        void updateCascades(const vsg::Camera& camera, Pipeline::Data::Scene& data, const vsg::vec3& lightPos, float maxZ, float cascadeSplitLambda);

        vsg::ref_ptr<vsg::View> cascadeView(int i);
        vsg::ref_ptr<vsg::RenderGraph> renderGraph(int i);
    };
}

#endif
