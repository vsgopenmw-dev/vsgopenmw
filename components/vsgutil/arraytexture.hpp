#ifndef VSGOPENMW_VSGUTIL_ARRAYTEXTURE_H
#define VSGOPENMW_VSGUTIL_ARRAYTEXTURE_H

#include <iostream>
#include <stdexcept>

#include <vsg/core/Array2D.h>

#include "name.hpp"

namespace vsgUtil
{
    /*
     * Converts textures to array texture to circumvent resource limits and descriptor indexing limitations.
     */
    inline vsg::ref_ptr<vsg::Data> convertToArrayTexture(const vsg::DataList& images)
    {
        auto dataSizeWithoutMipmaps = [](const vsg::Data& data) -> auto {
            return data.width() * data.height() * data.properties.stride;
        };

        auto width = images[0]->width();
        auto height = images[0]->height();
        auto depth = images.size();
        auto dataSize = dataSizeWithoutMipmaps(*images[0]);
        auto properties = images[0]->properties;
        properties.imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        properties.maxNumMipmaps = 0; // .generateMipmaps = true;
        vsg::ref_ptr<vsg::Data> array;
        if (images[0]->valueSize() == 8)
            array = vsg::block64Array3D::create(width, height, depth, properties);
        else if (images[0]->valueSize() == 16)
            array = vsg::block128Array3D::create(width, height, depth, properties);
        else if (properties.format == VK_FORMAT_R8G8B8A8_UNORM)
            array = vsg::ubvec4Array3D::create(width, height, depth, properties);
        if (!array)
            throw std::runtime_error("!convertToArrayTexture<" + std::string(typeid(*images[0]).name()) + ">(VkFormat=" + std::to_string(properties.format) + ")");
        else if (array->dataSize() != dataSize * depth)
            throw std::runtime_error("!convertToArrayTexture<" + std::string(typeid(*images[0]).name()) + ">(VkFormat=" + std::to_string(properties.format) + ", array->dataSize=" + std::to_string(array->dataSize()) + ", dataSize*depth=" + std::to_string(dataSize*depth) + ")");

        auto pDst = reinterpret_cast<uint8_t*>(array->dataPointer());
        for (size_t i = 0; i < depth; ++i)
        {
            auto& image = images[i];
            auto pSrc = reinterpret_cast<uint8_t*>(image->dataPointer());
            if (image->width() != width || image->height() != height || dataSizeWithoutMipmaps(*image) != dataSize || image->properties.format != properties.format)
            {
                std::cerr << "!convertToArrayTexture(i=" << i << ", name=" << vsgUtil::getName(*image) << "): This layer is unsupported. Layers must have matching sizes and formats." << std::endl;
            }
            else
                std::memcpy(pDst, pSrc, dataSize);
            pDst += dataSize;
        }
        return array;
    }
}

#endif
