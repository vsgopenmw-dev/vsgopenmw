#include "decompress.hpp"

#include <iostream>

#include <vsg/core/Array2D.h>
#include <vsg/core/Array3D.h>

namespace
{
    // based on: dxtctool.h: interface for the DXTC tools.
    //
    //////////////////////////////////////////////////////////////////////
    //
    //  Copyright (C) 2002 Tanguy Fautr.
    //
    //  This software is provided 'as-is', without any express or implied
    //  warranty.  In no event will the authors be held liable for any damages
    //  arising from the use of this software.
    //
    //  Permission is granted to anyone to use this software for any purpose,
    //  including commercial applications, and to alter it and redistribute it
    //  freely, subject to the following restrictions:
    //
    //  1. The origin of this software must not be misrepresented; you must not
    //     claim that you wrote the original software. If you use this software
    //     in a product, an acknowledgment in the product documentation would be
    //     appreciated but is not required.
    //  2. Altered source versions must be plainly marked as such, and must not be
    //     misrepresented as being the original software.
    //  3. This notice may not be removed or altered from any source distribution.
    //
    //  Tanguy Fautr
    //  softdev@telenet.be

    unsigned short interpolateColors21(unsigned short color1, unsigned short color2)
    {
        unsigned short result = (((color1 >> 11) * 2 + (color2 >> 11) + 1) / 3) << 11;
        result += (((color1 >> 5 & 0x3F) * 2 + (color2 >> 5 & 0x3F) + 1) / 3) << 5;
        result += (((color1 & 0x1F) * 2 + (color2 & 0x1F) + 1) / 3);
        return result;
    }
    unsigned short interpolateColors11(unsigned short color1, unsigned short color2)
    {
        unsigned short result = (((color1 >> 11) + (color2 >> 11)) / 2) << 11;
        result += (((color1 >> 5 & 0x3F) + (color2 >> 5 & 0x3F)) / 2) << 5;
        result += (((color1 & 0x1F) + (color2 & 0x1F)) / 2);
        return result;
    }

    //
    // Structure of a DXT-1 compressed texture block
    // see page "Opaque and 1-Bit Alpha Textures (Direct3D 9)" on http://msdn.microsoft.com
    // url at time of writing http://msdn.microsoft.com/en-us/library/bb147243(v=VS.85).aspx
    //
    struct DXT1TexelsBlock
    {
        unsigned short color_0; // colors at their
        unsigned short color_1; // extreme
        unsigned int texels4x4; // interpolated colors (2 bits per texel)
    };
    struct DXT3TexelsBlock
    {
        unsigned short alpha4[4]; // alpha values (4 bits per texel) - 64 bits
        unsigned short color_0; // colors at their
        unsigned short color_1; // extreme
        unsigned int texels4x4; // interpolated colors (2 bits per texel)
    };

    struct DXT5TexelsBlock
    {
        unsigned char alpha_0; // alpha at their
        unsigned char alpha_1; // extreme
        unsigned char alpha3[6]; // alpha index values (3 bits per texel)
        unsigned short color_0; // colors at their
        unsigned short color_1; // extreme
        unsigned int texels4x4; // interpolated colors (2 bits per texel)
    };

    bool compressedImageGetColor(unsigned char color[4], unsigned int s, unsigned int t, unsigned int r, int width,
        int height, int depth, VkFormat format, const unsigned char* imageData)
    {
        unsigned short color16 = 0; // RGB 5:6:5 format

        unsigned int slab4Count = (depth & ~0x3); // 4*floor(d/4)
        unsigned int col = (s >> 2); //(floor(x/4)
        unsigned int row = (t >> 2); //(floor(y/4)
        unsigned int blockWidth = (width + 3) >> 2; // ceil(w/4)
        unsigned int blockHeight = (height + 3) >> 2; // ceil(h/4)
        int blockNumber = col + blockWidth * row; // block to jump to

        if (depth > 1)
        {
            // https://www.opengl.org/registry/specs/NV/texture_compression_vtc.txt
            //    if (z >= 4*floor(d/4)) {
            //        blockIndex = blocksize * (ceil(w/4) * ceil(h/4) * 4*floor(d/4) + floor(x/4) + ceil(w/4) *
            //        (floor(y/4) + ceil(h/4) * (z-4*floor(d/4)) ));
            //    } else {
            //        blockIndex = blocksize * 4 * (floor(x/4) + ceil(w/4) * (floor(y/4) + ceil(h/4) * floor(z/4)));
            //    }
            // note floor(a/4) = (a >> 2)
            // note 4*floor(a/4) = a & ~0x3
            // note ceil(a/4) = ((a + 3) >> 2)
            //
            //  rewrite: this describes the final blocks as consecutive 4x4x1 blocks - and thats not in the wording of
            //  the specs
            //    if (r >= slab4Count) {
            //        blockNumber = (blockWidth * blockHeight * slab4Count  + col + blockWidth * (row + blockHeight *
            //        (r-slab4Count) ));
            //    } else {
            //      blockNumber = 4 * (col + blockWidth * (row + blockHeight * (r >> 2)) );
            //    }

            // or in the version of the openGL specs:
            //    if (z >= 4*floor(d/4)) {
            //        blockIndex = blocksize * (ceil(w/4) * ceil(h/4) * 4*floor(d/4) + (z - 4*floor(d/4)) * (
            //        (floor(x/4) + ceil(w/4) * (floor(y/4) );
            //    } else {
            //        blockIndex = blocksize * 4 * (floor(x/4) + ceil(w/4) * (floor(y/4) + ceil(h/4) * floor(z/4)));
            //    }

            unsigned int sub_r = r & 0x3; //(r-slab4Count)
            if (r >= slab4Count)
            { // slice number beyond  4x4x4 slabs
                unsigned int blockDepth = depth & 0x3; // equals: depth - slab4Count;//depth of this final block: 1/2/3
                                                       // in case of 4x4x1; 4x4x2 or 4x4x3 bricks
                blockNumber = (blockWidth * blockHeight * slab4Count // jump full 4x4x4 slabs
                    + blockDepth * (col + blockWidth * row) + sub_r);
            }
            else
            {
                blockNumber = 4 * (col + blockWidth * (row + blockHeight * (r >> 2))) + sub_r;
            }
        }

        int sub_s = s & 0x3;
        int sub_t = t & 0x3;
        switch (format)
        {
            case (VK_FORMAT_BC1_RGB_SRGB_BLOCK):
            case (VK_FORMAT_BC1_RGB_UNORM_BLOCK):
            case (VK_FORMAT_BC1_RGBA_SRGB_BLOCK):
            case (VK_FORMAT_BC1_RGBA_UNORM_BLOCK):
            {
                const DXT1TexelsBlock* texelsBlock = reinterpret_cast<const DXT1TexelsBlock*>(imageData);
                texelsBlock += blockNumber; // jump to block
                char index = (texelsBlock->texels4x4 >> (2 * sub_s + 8 * sub_t)) & 0x3; // two bit "index value"
                color[3] = 255;
                switch (index)
                {
                    case 0:
                        color16 = texelsBlock->color_0;
                        break;
                    case 1:
                        color16 = texelsBlock->color_1;
                        break;
                    case 2:
                        if (texelsBlock->color_0 > texelsBlock->color_1)
                        {
                            color16 = interpolateColors21(texelsBlock->color_0, texelsBlock->color_1);
                        }
                        else
                        {
                            color16 = interpolateColors11(texelsBlock->color_0, texelsBlock->color_1);
                        }
                        break;
                    case 3:
                        if (texelsBlock->color_0 > texelsBlock->color_1)
                        {
                            color16 = interpolateColors21(texelsBlock->color_1, texelsBlock->color_0);
                        }
                        else
                        {
                            color16 = 0; // black
                            if (format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK || format == VK_FORMAT_BC1_RGBA_SRGB_BLOCK)
                                color[3] = 0; // transparent
                        }
                        break;
                }
                break;
            }
            case (VK_FORMAT_BC2_UNORM_BLOCK):
            case (VK_FORMAT_BC2_SRGB_BLOCK):
            {
                const DXT3TexelsBlock* texelsBlock = reinterpret_cast<const DXT3TexelsBlock*>(imageData);
                texelsBlock += blockNumber; // jump to block
                color[3] = 17 * (texelsBlock->alpha4[sub_t] >> 4 * sub_s & 0xF);
                char index = (texelsBlock->texels4x4 >> (2 * sub_s + 8 * sub_t)) & 0x3; // two bit "index value"
                switch (index)
                {
                    case 0:
                        color16 = texelsBlock->color_0;
                        break;
                    case 1:
                        color16 = texelsBlock->color_1;
                        break;
                    case 2:
                        color16 = interpolateColors21(texelsBlock->color_0, texelsBlock->color_1);
                        break;
                    case 3:
                        color16 = interpolateColors21(texelsBlock->color_1, texelsBlock->color_0);
                        break;
                }
                break;
            }
            case (VK_FORMAT_BC3_UNORM_BLOCK):
            case (VK_FORMAT_BC3_SRGB_BLOCK):
            {
                const DXT5TexelsBlock* texelsBlock = reinterpret_cast<const DXT5TexelsBlock*>(imageData);
                texelsBlock += blockNumber; // jump to block
                char index = (texelsBlock->texels4x4 >> (2 * sub_s + 8 * sub_t)) & 0x3; // two bit "index value"
                switch (index)
                {
                    case 0:
                        color16 = texelsBlock->color_0;
                        break;
                    case 1:
                        color16 = texelsBlock->color_1;
                        break;
                    case 2:
                        color16 = interpolateColors21(texelsBlock->color_0, texelsBlock->color_1);
                        break;
                    case 3:
                        color16 = interpolateColors21(texelsBlock->color_1, texelsBlock->color_0);
                        break;
                }
                char pixel = sub_s + 4 * sub_t; // pixel number in block: 0 - 15
                char firstBit = 3 * pixel; // least significant bit: range 0 - 45
                unsigned char alpha_index;
                if ((firstBit & 0x7) < 6)
                {
                    alpha_index = texelsBlock->alpha3[firstBit >> 3] >> (firstBit & 0x7)
                        & 0x7; // grab byte containing least significant bit; shift and get 3 bits
                }
                else
                {
                    alpha_index = texelsBlock->alpha3[firstBit >> 3] >> (firstBit & 0x7);
                    alpha_index |= texelsBlock->alpha3[1 + (firstBit >> 3)] << (8 - (firstBit & 0x7));
                    alpha_index &= 0x7;
                }
                if (alpha_index == 0)
                {
                    color[3] = texelsBlock->alpha_0;
                }
                else
                {
                    if (alpha_index == 1)
                    {
                        color[3] = texelsBlock->alpha_1;
                    }
                    else
                    {
                        if (texelsBlock->alpha_0 > texelsBlock->alpha_1)
                        {
                            color[3] = ((unsigned short)texelsBlock->alpha_0 * (8 - alpha_index)
                                           + (unsigned short)texelsBlock->alpha_1 * (alpha_index - 1) + 3)
                                / 7;
                        }
                        else
                        {
                            if (alpha_index < 6)
                            {
                                color[3] = ((unsigned short)texelsBlock->alpha_0 * (6 - alpha_index)
                                               + (unsigned short)texelsBlock->alpha_1 * (alpha_index - 1) + 3)
                                    / 5;
                            }
                            else
                            {
                                if (alpha_index == 6)
                                {
                                    color[3] = 0;
                                }
                                else
                                {
                                    color[3] = 255;
                                }
                            }
                        }
                    }
                }
                break;
            }
            default:
            {
                std::cerr << "!CompressedImageGetColor(VkFormat=" << format << ")" << std::endl;
                return false;
            }
        }
        unsigned short colorChannel = color16 >> 11; // red - 5 bits
        color[0] = colorChannel << 3 | colorChannel >> 2;
        colorChannel = color16 >> 5 & 0x3F; // green - 6 bits
        color[1] = colorChannel << 2 | colorChannel >> 3;
        colorChannel = color16 & 0x1F; // blue - 5 bits
        color[2] = colorChannel << 3 | colorChannel >> 2;
        return true;
    }
}

namespace vsgUtil
{
    vsg::ref_ptr<vsg::Data> decompressImage(const vsg::Data& in_data)
    {
        if (!isCompressed(in_data))
            return {};
        const auto& layout = in_data.properties;
        auto w = in_data.width() * layout.blockWidth;
        auto h = in_data.height() * layout.blockHeight;
        auto d = in_data.depth() * layout.blockDepth;
        vsg::ref_ptr<vsg::Data> newData;
        if (d == 1)
            newData = vsg::ubvec4Array2D::create(w, h, vsg::Data::Properties{ VK_FORMAT_R8G8B8A8_UNORM });
        else
            newData = vsg::ubvec4Array3D::create(w, h, d, vsg::Data::Properties{ VK_FORMAT_R8G8B8A8_UNORM });

        for (std::uint32_t x = 0; x < w; ++x)
        {
            for (std::uint32_t y = 0; y < h; ++y)
            {
                for (std::uint32_t z = 0; z < d; ++z)
                {
                    unsigned char color[4];
                    if (!compressedImageGetColor(color, x, y, z, w, h, d, layout.format,
                            reinterpret_cast<const unsigned char*>(in_data.dataPointer())))
                        return {};
                    std::memcpy(newData->dataPointer(z * w * h + y * w + x), &color[0], sizeof(vsg::ubvec4));
                }
            }
        }
        return newData;
    }

    bool isCompressed(const vsg::Data& in_data)
    {
        const auto& layout = in_data.properties;
        return !(layout.blockWidth == 1 && layout.blockHeight == 1 && layout.blockDepth == 1);
    }
}
