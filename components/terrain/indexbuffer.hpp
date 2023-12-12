#ifndef VSGOPENMW_TERRAIN_INDEXBUFFER_H
#define VSGOPENMW_TERRAIN_INDEXBUFFER_H

#include <cassert>

#include <vsg/commands/BindIndexBuffer.h>

namespace Terrain
{
    using IndexBufferKey = std::pair<uint32_t /*numVerts*/, uint32_t /*flags*/>;

    struct CreateIndexBuffer
    {
        enum Direction
        {
            North = 0,
            East = 1,
            South = 2,
            West = 3
        };

        vsg::ref_ptr<vsg::BindIndexBuffer> create(const IndexBufferKey& key) const
        {
            auto numVerts = key.first;
            if (numVerts * numVerts <= 0xffffu)
                return t_create<vsg::ushortArray>(key);
            else
                return t_create<vsg::uintArray>(key);
        }

        template <class IndexArrayType>
        vsg::ref_ptr<vsg::BindIndexBuffer> t_create(const IndexBufferKey& key) const
        {
            auto verts = key.first;
            size_t lodDeltas[4];
            for (int i = 0; i < 4; ++i)
                lodDeltas[i] = (key.second >> (4 * i)) & (0xf);
            bool anyDeltas = (lodDeltas[North] || lodDeltas[South] || lodDeltas[West] || lodDeltas[East]);
            size_t indexCount = (verts - 1) * (verts - 1) * 2 * 3;

            for (int i=0; i<4; ++i)
            {
                if (lodDeltas[i])
                {
                    size_t outer = (verts - 1) * 3 / (1 << lodDeltas[i]);
                    size_t inner = (verts - 1) * 3;
                    indexCount -= inner - outer;
                }
            }
            auto indices = IndexArrayType::create(indexCount);
            auto ptr = indices->data();
            size_t rowStart = 0, colStart = 0, rowEnd = verts - 1, colEnd = verts - 1;
            // If any edge needs stitching we'll skip all edges at this point,
            // mainly because stitching one edge would have an effect on corners and on the adjacent edges
            if (anyDeltas)
            {
                ++colStart;
                ++rowStart;
                --colEnd;
                --rowEnd;
            }
            for (size_t row = rowStart; row < rowEnd; ++row)
            {
                for (size_t col = colStart; col < colEnd; ++col)
                {
                    // diamond pattern
                    if ((row + col % 2) % 2 == 1)
                    {
                        *(ptr++) = (verts * (col + 1) + row);
                        *(ptr++) = (verts * (col + 1) + row + 1);
                        *(ptr++) = (verts * col + row + 1);

                        *(ptr++) = (verts * col + row);
                        *(ptr++) = (verts * (col + 1) + row);
                        *(ptr++) = (verts * (col) + row + 1);
                    }
                    else
                    {
                        *(ptr++) = (verts * col + row);
                        *(ptr++) = (verts * (col + 1) + row + 1);
                        *(ptr++) = (verts * col + row + 1);

                        *(ptr++) = (verts * col + row);
                        *(ptr++) = (verts * (col + 1) + row);
                        *(ptr++) = (verts * (col + 1) + row + 1);
                    }
                }
            }

            if (anyDeltas)
            {
                // Now configure LOD transitions at the edges - this is pretty tedious,
                // and some very long and boring code, but it works great

                // South
                size_t row = 0;
                size_t outerStep = static_cast<size_t>(1) << lodDeltas[South];
                for (size_t col = 0; col < verts - 1; col += outerStep)
                {
                    *(ptr++) = (verts * col + row);
                    *(ptr++) = (verts * (col + outerStep) + row);
                    // Make sure not to touch the right edge
                    if (col + outerStep == verts - 1)
                        *(ptr++) = (verts * (col + outerStep - 1) + row + 1);
                    else
                        *(ptr++) = (verts * (col + outerStep) + row + 1);

                    for (size_t i = 0; i < outerStep; i += 1)
                    {
                        // Make sure not to touch the left or right edges
                        if (col + i == 0 || col + i == verts - 1 - 1)
                            continue;
                        *(ptr++) = (verts * (col) + row);
                        *(ptr++) = (verts * (col + i + 1) + row + 1);
                        *(ptr++) = (verts * (col + i) + row + 1);
                    }
                }

                // North
                row = verts - 1;
                outerStep = size_t(1) << lodDeltas[North];
                for (size_t col = 0; col < verts - 1; col += outerStep)
                {
                    *(ptr++) = (verts * (col + outerStep) + row);
                    *(ptr++) = (verts * col + row);
                    // Make sure not to touch the left edge
                    if (col == 0)
                        *(ptr++) = (verts * (col + 1) + row - 1);
                    else
                        *(ptr++) = (verts * col + row - 1);

                    for (size_t i = 0; i < outerStep; i += 1)
                    {
                        // Make sure not to touch the left or right edges
                        if (col + i == 0 || col + i == verts - 1 - 1)
                            continue;
                        *(ptr++) = (verts * (col + i) + row - 1);
                        *(ptr++) = (verts * (col + i + 1) + row - 1);
                        *(ptr++) = (verts * (col + outerStep) + row);
                    }
                }

                // West
                size_t col = 0;
                outerStep = size_t(1) << lodDeltas[West];
                for (row = 0; row < verts - 1; row += outerStep)
                {
                    *(ptr++) = (verts * col + row + outerStep);
                    *(ptr++) = (verts * col + row);
                    // Make sure not to touch the top edge
                    if (row + outerStep == verts - 1)
                        *(ptr++) = (verts * (col + 1) + row + outerStep - 1);
                    else
                        *(ptr++) = (verts * (col + 1) + row + outerStep);

                    for (size_t i = 0; i < outerStep; i += 1)
                    {
                        // Make sure not to touch the top or bottom edges
                        if (row + i == 0 || row + i == verts - 1 - 1)
                            continue;
                        *(ptr++) = (verts * col + row);
                        *(ptr++) = (verts * (col + 1) + row + i);
                        *(ptr++) = (verts * (col + 1) + row + i + 1);
                    }
                }

                // East
                col = verts - 1;
                outerStep = size_t(1) << lodDeltas[East];
                for (row = 0; row < verts - 1; row += outerStep)
                {
                    *(ptr++) = (verts * col + row);
                    *(ptr++) = (verts * col + row + outerStep);
                    // Make sure not to touch the bottom edge
                    if (row == 0)
                        *(ptr++) = (verts * (col - 1) + row + 1);
                    else
                        *(ptr++) = (verts * (col - 1) + row);

                    for (size_t i = 0; i < outerStep; i += 1)
                    {
                        // Make sure not to touch the top or bottom edges
                        if (row + i == 0 || row + i == verts - 1 - 1)
                            continue;
                        *(ptr++) = (verts * col + row + outerStep);
                        *(ptr++) = (verts * (col - 1) + row + i + 1);
                        *(ptr++) = (verts * (col - 1) + row + i);
                    }
                }
            }
            assert(static_cast<size_t>(ptr - indices->data()) == indices->valueCount());
            return vsg::BindIndexBuffer::create(indices);
        }
    };
}

#endif
