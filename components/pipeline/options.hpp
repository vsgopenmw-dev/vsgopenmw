#ifndef VSGOPENMW_PIPELINE_OPTIONS_H
#define VSGOPENMW_PIPELINE_OPTIONS_H

#include <compare>
#include <map>
#include <string>

#include <vsg/vk/vulkan.h>

#include "mode.hpp"

namespace Pipeline
{
    /*
     * Affects shader source.
     */
    struct ShaderOptions
    {
        std::string path;
        std::vector<bool> modes;
        /*auto*/ std::strong_ordering operator<=>(const ShaderOptions&) const = default;

        void addMode(Mode mode) { Pipeline::addMode(mode, modes); }
        bool hasMode(Mode mode) const { return Pipeline::hasMode(mode, modes); }
    };

    /*
     * Configures graphics pipeline.
     */
    struct Options
    {
        ShaderOptions shader;
        uint32_t numUvSets = 0;
        std::map<uint32_t, uint32_t> nonStandardUvSets;
        VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        bool depthTest = true;
        bool depthWrite = true;
        bool specular = false;
        bool blend = false;
        bool colorWrite = true;
        bool depthBias = false;
        VkBlendFactor srcBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        VkBlendFactor dstBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        uint32_t alphaTestMode = 0;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        uint32_t colorMode = 0;

        /*auto*/ std::strong_ordering operator<=>(const Options&) const = default;
    };
}

#endif
