#ifndef VSGOPENMW_PIPELINE_OPTIONS_H
#define VSGOPENMW_PIPELINE_OPTIONS_H

#include <vsg/state/GraphicsPipeline.h>

#include "mode.hpp"

namespace Pipeline
{
    /*
     * Configures graphics pipeline.
     */
    struct Options
    {
        std::string shader;
        std::vector<bool> modes;
        std::vector<bool> geometryModes;
        //std::vector<unsigned int> arrayModes;
        size_t numUvSets = 0;
        std::map<uint32_t, uint32_t> nonStandardUvSets;
        VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        bool depthTest = true;
        bool depthWrite = true;
        bool specular = false;
        bool blend = false;
        bool colorWrite = true;
        VkBlendFactor srcBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        VkBlendFactor dstBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        uint32_t alphaTestMode = 0;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        uint32_t colorMode = 0;

        auto operator<=>(const Options&) const = default;
        template <class M>
        void addMode(M mode, std::vector<bool> &vec)
        {
            auto i = static_cast<size_t>(mode);
            if (i >= vec.size())
                vec.resize(i+1);
            vec[i] = true;
        }
        template <class M>
        bool hasMode(M mode, const std::vector<bool> &vec) const
        {
            auto i = static_cast<size_t>(mode);
            return i < vec.size() && vec[i];
        }
        void addMode(GeometryMode mode)
        {
            addMode(mode, geometryModes);
        }
        bool hasMode(GeometryMode mode) const
        {
            return hasMode(mode, geometryModes);
        }
        void addMode(Mode mode)
        {
            addMode(mode, modes);
        }
        bool hasMode(Mode mode) const
        {
            return hasMode(mode, modes);
        }
    };
}

#endif
