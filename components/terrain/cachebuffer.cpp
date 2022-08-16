#include "cachebuffer.hpp"

#include <cassert>

#include "defs.hpp"

namespace
{
    template <class IndexArrayType>
    struct CreateIndexBuffer
    {
        vsg::ref_ptr<vsg::BindIndexBuffer> create(const Terrain::IndexBufferKey &key) const
        {
            auto verts = key.numVerts;
            // LOD level n means every 2^n-th vertex is kept, but we currently handle LOD elsewhere.
            size_t lodLevel = 0;//(flags >> (4*4));
            size_t lodDeltas[4];
            for (int i=0; i<4; ++i)
                lodDeltas[i] = (key.flags >> (4*i)) & (0xf);
            bool anyDeltas = (lodDeltas[Terrain::North] || lodDeltas[Terrain::South] || lodDeltas[Terrain::West] || lodDeltas[Terrain::East]);
            size_t increment = static_cast<size_t>(1) << lodLevel;
            assert(increment < verts);
            auto indices = IndexArrayType::create((verts-1)*(verts-1)*2*3 / increment);
            auto ptr = indices->data();
            size_t rowStart = 0, colStart = 0, rowEnd = verts-1, colEnd = verts-1;
            // If any edge needs stitching we'll skip all edges at this point,
            // mainly because stitching one edge would have an effect on corners and on the adjacent edges
            if (anyDeltas)
            {
                colStart += increment;
                colEnd -= increment;
                rowEnd -= increment;
                rowStart += increment;
            }
            for (size_t row = rowStart; row < rowEnd; row += increment)
            {
                for (size_t col = colStart; col < colEnd; col += increment)
                {
                    // diamond pattern
                    if ((row + col%2) % 2 == 1)
                    {
                        *(ptr++) = (verts*(col+increment)+row);
                        *(ptr++) = (verts*(col+increment)+row+increment);
                        *(ptr++) = (verts*col+row+increment);

                        *(ptr++) = (verts*col+row);
                        *(ptr++) = (verts*(col+increment)+row);
                        *(ptr++) = (verts*(col)+row+increment);
                    }
                    else
                    {
                        *(ptr++) = (verts*col+row);
                        *(ptr++) = (verts*(col+increment)+row+increment);
                        *(ptr++) = (verts*col+row+increment);

                        *(ptr++) = (verts*col+row);
                        *(ptr++) = (verts*(col+increment)+row);
                        *(ptr++) = (verts*(col+increment)+row+increment);
                    }
                }
            }

            size_t innerStep = increment;
            if (anyDeltas)
            {
                // Now configure LOD transitions at the edges - this is pretty tedious,
                // and some very long and boring code, but it works great

                // South
                size_t row = 0;
                size_t outerStep = static_cast<size_t>(1) << (lodDeltas[Terrain::South] + lodLevel);
                for (size_t col = 0; col < verts-1; col += outerStep)
                {
                    *(ptr++) = (verts*col+row);
                    *(ptr++) = (verts*(col+outerStep)+row);
                    // Make sure not to touch the right edge
                    if (col+outerStep == verts-1)
                        *(ptr++) = (verts*(col+outerStep-innerStep)+row+innerStep);
                    else
                        *(ptr++) = (verts*(col+outerStep)+row+innerStep);

                    for (size_t i = 0; i < outerStep; i += innerStep)
                    {
                        // Make sure not to touch the left or right edges
                        if (col+i == 0 || col+i == verts-1-innerStep)
                            continue;
                        *(ptr++) = (verts*(col)+row);
                        *(ptr++) = (verts*(col+i+innerStep)+row+innerStep);
                        *(ptr++) = (verts*(col+i)+row+innerStep);
                    }
                }

                // North
                row = verts-1;
                outerStep = size_t(1) << (lodDeltas[Terrain::North] + lodLevel);
                for (size_t col = 0; col < verts-1; col += outerStep)
                {
                    *(ptr++) = (verts*(col+outerStep)+row);
                    *(ptr++) = (verts*col+row);
                    // Make sure not to touch the left edge
                    if (col == 0)
                        *(ptr++) = (verts*(col+innerStep)+row-innerStep);
                    else
                        *(ptr++) = (verts*col+row-innerStep);

                    for (size_t i = 0; i < outerStep; i += innerStep)
                    {
                        // Make sure not to touch the left or right edges
                        if (col+i == 0 || col+i == verts-1-innerStep)
                            continue;
                        *(ptr++) = (verts*(col+i)+row-innerStep);
                        *(ptr++) = (verts*(col+i+innerStep)+row-innerStep);
                        *(ptr++) = (verts*(col+outerStep)+row);
                    }
                }

                // West
                size_t col = 0;
                outerStep = size_t(1) << (lodDeltas[Terrain::West] + lodLevel);
                for (row = 0; row < verts-1; row += outerStep)
                {
                    *(ptr++) = (verts*col+row+outerStep);
                    *(ptr++) = (verts*col+row);
                    // Make sure not to touch the top edge
                    if (row+outerStep == verts-1)
                        *(ptr++) = (verts*(col+innerStep)+row+outerStep-innerStep);
                    else
                        *(ptr++) = (verts*(col+innerStep)+row+outerStep);

                    for (size_t i = 0; i < outerStep; i += innerStep)
                    {
                        // Make sure not to touch the top or bottom edges
                        if (row+i == 0 || row+i == verts-1-innerStep)
                            continue;
                        *(ptr++) = (verts*col+row);
                        *(ptr++) = (verts*(col+innerStep)+row+i);
                        *(ptr++) = (verts*(col+innerStep)+row+i+innerStep);
                    }
                }

                // East
                col = verts-1;
                outerStep = size_t(1) << (lodDeltas[Terrain::East] + lodLevel);
                for (row = 0; row < verts-1; row += outerStep)
                {
                    *(ptr++) = (verts*col+row);
                    *(ptr++) = (verts*col+row+outerStep);
                    // Make sure not to touch the bottom edge
                    if (row == 0)
                        *(ptr++) = (verts*(col-innerStep)+row+innerStep);
                    else
                        *(ptr++) = (verts*(col-innerStep)+row);

                    for (size_t i = 0; i < outerStep; i += innerStep)
                    {
                        // Make sure not to touch the top or bottom edges
                        if (row+i == 0 || row+i == verts-1-innerStep)
                            continue;
                        *(ptr++) = (verts*col+row+outerStep);
                        *(ptr++) = (verts*(col-innerStep)+row+i+innerStep);
                        *(ptr++) = (verts*(col-innerStep)+row+i);
                    }
                }
            }
            return vsg::BindIndexBuffer::create(indices);
        }
    };

    struct CreateUvBuffer
    {
        vsg::ref_ptr<vsg::Data> create(unsigned int numVerts) const
        {
            auto vertexCount = numVerts * numVerts;
            auto uvs = vsg::vec2Array::create(vertexCount);
            auto ptr = uvs->data();
            for (unsigned int col = 0; col < numVerts; ++col)
            {
                for (unsigned int row = 0; row < numVerts; ++row)
                {
                    *(ptr++) = vsg::vec2(col / static_cast<float>(numVerts-1), ((numVerts-1) - row) / static_cast<float>(numVerts-1));
                }
            }
            return uvs;
        }
    };
}

namespace Terrain
{
    vsg::ref_ptr<vsg::Data> CacheBuffer::getUVBuffer(unsigned int numVerts)
    {
        return mUvBufferCache.getOrCreate(numVerts, CreateUvBuffer());
    }

    vsg::ref_ptr<vsg::BindIndexBuffer> CacheBuffer::getIndexBuffer(unsigned int numVerts, unsigned int flags)
    {
        if (numVerts*numVerts <= (0xffffu))
            return mIndexBufferCache.getOrCreate({numVerts, flags}, CreateIndexBuffer<vsg::ushortArray>());
        else

            return mIndexBufferCache.getOrCreate({numVerts, flags}, CreateIndexBuffer<vsg::uintArray>());
    }
}
