#ifndef VSGOPENMW_MWRENDER_RENDERPASS_H
#define VSGOPENMW_MWRENDER_RENDERPASS_H

#include <components/render/renderpass.hpp>
#include <components/render/attachmentformat.hpp>

namespace MWRender
{
    vsg::ref_ptr<vsg::RenderPass> createMainRenderPass(vsg::Device* device, VkSampleCountFlagBits samples, VkImageLayout presentLayout)
    {
        return Render::createMultisampledRenderPass(device, Render::compatibleColorFormat, Render::compatibleDepthFormat, samples, false, true, presentLayout); // supportReflectionDepth, supportScreenshot
    }

    vsg::ref_ptr<vsg::RenderPass> createGuiRenderPass(vsg::Device* device, VkSampleCountFlagBits samples, VkImageLayout presentLayout)
    {
        return Render::createMultisampledRenderPass(device, Render::compatibleColorFormat, Render::compatibleDepthFormat, samples, true, false, presentLayout);
    }
}

#endif
