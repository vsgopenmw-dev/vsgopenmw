#ifndef VSGOPENMW_RENDER_RENDERTEXTURE_H
#define VSGOPENMW_RENDER_RENDERTEXTURE_H

#include <vsg/viewer/RenderGraph.h>

namespace Render //namespace vsgExamples
{
    vsg::ref_ptr<vsg::RenderGraph> createRenderTexture(vsg::Context &context, const VkExtent2D &extent, vsg::ref_ptr<vsg::ImageView> &out_imageView, int usage=VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout finalColorLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

#endif
