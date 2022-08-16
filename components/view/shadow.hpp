#ifndef VSGOPENMW_VIEW_SHADOW_H
#define VSGOPENMW_VIEW_SHADOW_H

#include <memory>
#include <vector>

#include <vsg/nodes/Node.h>

namespace Pipeline
{
    class Scene;
}
namespace vsg
{
    class Context;
}
namespace View //namespace vulkanexamples
{
    class Cascade;
    class Descriptors;

    /*
     * Renders shadow map.
     */
    class Shadow
    {
        std::vector<std::unique_ptr<Cascade>> mCascades;
        vsg::ref_ptr<vsg::Descriptor> mShadowMap;
        std::unique_ptr<Descriptors> mDescriptors;
        vsg::ref_ptr<vsg::View> mBaseView;
    public:
        Shadow(vsg::Context &ctx, int in_numCascades, uint32_t res);
        ~Shadow();
        const uint32_t resolution;
        const int numCascades;

        vsg::Descriptor *shadowMap();

        void updateCascades(const vsg::Camera &camera, Pipeline::Scene &data, const vsg::vec3 &lightPos, float far);

        vsg::ref_ptr<vsg::View> cascadeView(int i, vsg::Mask mask);
        vsg::ref_ptr<vsg::RenderGraph> renderGraph(int i);

        void setCastShadowScene(vsg::ref_ptr<vsg::Node> scene);
    };
}

#endif
