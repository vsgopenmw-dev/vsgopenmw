#include "storage.hpp"

#include <iostream>

#include <vsg/core/Array2D.h>

#include <osg/Plane>

#include <components/vsgadapters/osgcompat.hpp>
#include <components/terrain/bounds.hpp>

namespace ESMTerrain
{
    class LandCache
    {
    public:
        using Map = std::map<std::pair<int, int>, vsg::ref_ptr<const LandObject>>;
        Map mMap;
    };

    LandObject::LandObject(const ESM::Land& land, int loadFlags)
        : mLand(&land)
        , mLoadFlags(loadFlags)
    {
        mLand->loadData(mLoadFlags, mData);
    }

    LandObject::~LandObject() {}

    const float defaultHeight = ESM::Land::DEFAULT_HEIGHT;

    Storage::Storage()
        : Terrain::Storage(static_cast<float>(ESM::Land::REAL_SIZE), static_cast<unsigned int>(ESM::Land::LAND_SIZE))
    {
    }

    void Storage::fixNormal(vsg::vec3& normal, int cellX, int cellY, int col, int row, LandCache& cache)
    {
        while (col >= ESM::Land::LAND_SIZE - 1)
        {
            ++cellY;
            col -= ESM::Land::LAND_SIZE - 1;
        }
        while (row >= ESM::Land::LAND_SIZE - 1)
        {
            ++cellX;
            row -= ESM::Land::LAND_SIZE - 1;
        }
        while (col < 0)
        {
            --cellY;
            col += ESM::Land::LAND_SIZE - 1;
        }
        while (row < 0)
        {
            --cellX;
            row += ESM::Land::LAND_SIZE - 1;
        }

        const LandObject* land = getLand(cellX, cellY, cache);
        const ESM::Land::LandData* data = land ? land->getData(ESM::Land::DATA_VNML) : nullptr;
        if (data)
        {
            normal = vsg::normalize(vsg::vec3(data->mNormals[col * ESM::Land::LAND_SIZE * 3 + row * 3],
                data->mNormals[col * ESM::Land::LAND_SIZE * 3 + row * 3 + 1],
                data->mNormals[col * ESM::Land::LAND_SIZE * 3 + row * 3 + 2]));
        }
        else
            normal = vsg::vec3(0, 0, 1);
    }

    void Storage::averageNormal(vsg::vec3& normal, int cellX, int cellY, int col, int row, LandCache& cache)
    {
        vsg::vec3 n1, n2, n3, n4;
        fixNormal(n1, cellX, cellY, col + 1, row, cache);
        fixNormal(n2, cellX, cellY, col - 1, row, cache);
        fixNormal(n3, cellX, cellY, col, row + 1, cache);
        fixNormal(n4, cellX, cellY, col, row - 1, cache);
        normal = vsg::normalize(n1 + n2 + n3 + n4);
    }

    void Storage::fixColour(vsg::ubvec4& color, int cellX, int cellY, int col, int row, LandCache& cache)
    {
        if (col == ESM::Land::LAND_SIZE - 1)
        {
            ++cellY;
            col = 0;
        }
        if (row == ESM::Land::LAND_SIZE - 1)
        {
            ++cellX;
            row = 0;
        }

        const LandObject* land = getLand(cellX, cellY, cache);
        const ESM::Land::LandData* data = land ? land->getData(ESM::Land::DATA_VCLR) : nullptr;
        if (data)
        {
            color = { data->mColours[col * ESM::Land::LAND_SIZE * 3 + row * 3],
                data->mColours[col * ESM::Land::LAND_SIZE * 3 + row * 3 + 1],
                data->mColours[col * ESM::Land::LAND_SIZE * 3 + row * 3 + 2], 0xff };
        }
        else
        {
            color = { 0xff, 0xff, 0xff, 0xff };
        }
    }

    Storage::VertexData Storage::getVertexData(int lodLevel, const Terrain::Bounds& bounds)
    {
        // LOD level n means every 2^n-th vertex is kept
        size_t increment = static_cast<size_t>(1) << lodLevel;

        int startCellX = static_cast<int>(std::floor(bounds.min.x));
        int startCellY = static_cast<int>(std::floor(bounds.min.y));

        auto numVerts = Storage::numVerts(bounds.size, lodLevel);

        auto heights = vsg::floatArray2D::create(numVerts, numVerts, vsg::Data::Properties{VK_FORMAT_R32_SFLOAT});
        auto normals = vsg::vec3Array2D::create(numVerts, numVerts, vsg::Data::Properties{VK_FORMAT_R32G32B32_SFLOAT});
        auto colours = vsg::ubvec4Array2D::create(numVerts, numVerts, vsg::Data::Properties{VK_FORMAT_R8G8B8A8_UNORM});

        vsg::vec3 normal;
        vsg::ubvec4 color;

        uint32_t vertY = 0;
        uint32_t vertX = 0;

        LandCache cache;

        //bool alteration = useAlteration();

        uint32_t vertY_ = 0; // of current cell corner
        for (int cellY = startCellY; cellY < startCellY + std::ceil(bounds.size); ++cellY)
        {
            uint32_t vertX_ = 0; // of current cell corner
            for (int cellX = startCellX; cellX < startCellX + std::ceil(bounds.size); ++cellX)
            {
                const LandObject* land = getLand(cellX, cellY, cache);
                const ESM::Land::LandData* heightData = nullptr;
                const ESM::Land::LandData* normalData = nullptr;
                const ESM::Land::LandData* colourData = nullptr;
                if (land)
                {
                    heightData = land->getData(ESM::Land::DATA_VHGT);
                    normalData = land->getData(ESM::Land::DATA_VNML);
                    colourData = land->getData(ESM::Land::DATA_VCLR);
                }

                int rowStart = 0;
                int colStart = 0;
                // Skip the first row / column unless we're at a chunk edge,
                // since this row / column is already contained in a previous cell
                // This is only relevant if we're creating a chunk spanning multiple cells
                if (vertY_ != 0)
                    colStart += increment;
                if (vertX_ != 0)
                    rowStart += increment;

                // Only relevant for chunks smaller than (contained in) one cell
                rowStart += (bounds.min.x - startCellX) * ESM::Land::LAND_SIZE;
                colStart += (bounds.min.y - startCellY) * ESM::Land::LAND_SIZE;
                int rowEnd = std::min(static_cast<int>(rowStart + std::min(1.f, bounds.size) * (ESM::Land::LAND_SIZE - 1) + 1),
                    static_cast<int>(ESM::Land::LAND_SIZE));
                int colEnd = std::min(static_cast<int>(colStart + std::min(1.f, bounds.size) * (ESM::Land::LAND_SIZE - 1) + 1),
                    static_cast<int>(ESM::Land::LAND_SIZE));

                vertY = vertY_;
                for (int col = colStart; col < colEnd; col += increment, ++vertY)
                {
                    vertX = vertX_;
                    for (int row = rowStart; row < rowEnd; row += increment, ++vertX)
                    {
                        int srcArrayIndex = col * ESM::Land::LAND_SIZE * 3 + row * 3;

                        assert(row >= 0 && row < ESM::Land::LAND_SIZE);
                        assert(col >= 0 && col < ESM::Land::LAND_SIZE);

                        assert(vertX < numVerts);
                        assert(vertY < numVerts);

                        float height = defaultHeight;
                        if (heightData)
                            height = heightData->mHeights[col * ESM::Land::LAND_SIZE + row];
                        //if (alteration)
                            ///height += getAlteredHeight(col, row);
                        (*heights)(vertX, vertY) = height;
                        if (normalData)
                        {
                            for (int i = 0; i < 3; ++i)
                                normal[i] = normalData->mNormals[srcArrayIndex + i];

                            normal = vsg::normalize(normal);
                        }
                        else
                            normal = vsg::vec3(0, 0, 1);

                        // Normals apparently don't connect seamlessly between cells
                        if (col == ESM::Land::LAND_SIZE - 1 || row == ESM::Land::LAND_SIZE - 1)
                            fixNormal(normal, cellX, cellY, col, row, cache);

                        // some corner normals appear to be complete garbage (z < 0)
                        if ((row == 0 || row == ESM::Land::LAND_SIZE - 1)
                            && (col == 0 || col == ESM::Land::LAND_SIZE - 1))
                            averageNormal(normal, cellX, cellY, col, row, cache);

                        //assert(normal.z() > 0);

                        (*normals)(vertX, vertY) = normal;

                        if (colourData)
                        {
                            color = { colourData->mColours[srcArrayIndex], colourData->mColours[srcArrayIndex + 1],
                                colourData->mColours[srcArrayIndex + 2], 0xff };
                        }
                        else
                            color = { 0xff, 0xff, 0xff, 0xff };
                        //if (alteration)
                            //adjustColor(col, row, heightData, color); // Does nothing by default, override in OpenMW-CS

                        // Unlike normals, colors mostly connect seamlessly between cells, but not always...
                        if (col == ESM::Land::LAND_SIZE - 1 || row == ESM::Land::LAND_SIZE - 1)
                            fixColour(color, cellX, cellY, col, row, cache);

                        (*colours)(vertX, vertY) = color;
                    }
                }
                vertX_ = vertX;
            }
            vertY_ = vertY;

            assert(vertX_ == numVerts); // Ensure we covered whole area
        }
        assert(vertY_ == numVerts); // Ensure we covered whole area
        return { heights, normals, colours };
    }

    Storage::PluginTexture Storage::getVtexIndexAt(int cellX, int cellY, int x, int y, LandCache& cache)
    {
        // For the first/last row/column, we need to get the texture from the neighbour cell
        // to get consistent blending at the borders
        --x;
        if (x < 0)
        {
            --cellX;
            x += ESM::Land::LAND_TEXTURE_SIZE;
        }
        while (x >= ESM::Land::LAND_TEXTURE_SIZE)
        {
            ++cellX;
            x -= ESM::Land::LAND_TEXTURE_SIZE;
        }
        while (
            y >= ESM::Land::LAND_TEXTURE_SIZE) // Y appears to be wrapped from the other side because why the hell not?
        {
            ++cellY;
            y -= ESM::Land::LAND_TEXTURE_SIZE;
        }

        assert(x < ESM::Land::LAND_TEXTURE_SIZE);
        assert(y < ESM::Land::LAND_TEXTURE_SIZE);

        const LandObject* land = getLand(cellX, cellY, cache);

        const ESM::Land::LandData* data = land ? land->getData(ESM::Land::DATA_VTEX) : nullptr;
        if (data)
        {
            int tex = data->mTextures[y * ESM::Land::LAND_TEXTURE_SIZE + x];
            return std::make_pair(tex, land->getPlugin());
        }
        return std::make_pair(0, 0);
    }

    std::vector<Terrain::LayerInfo> Storage::getLayers()
    {
        std::map<std::string, size_t> layerInfoMap;
        std::vector<Terrain::LayerInfo> layerInfos;
        auto textureMap = enumerateLayers();
        for (auto [id, name] : textureMap)
        {
            // look for existing diffuse map, which may be present when several plugins use the same texture
            auto found = layerInfoMap.find(name);
            auto index = layerInfos.size();
            if (found != layerInfoMap.end())
                index = found->second;
            else
            {
                layerInfoMap[name] = layerInfos.size();
                layerInfos.emplace_back(name);
            }
            if (static_cast<short>(mLayerIndices.size()) < id.first+1)
                mLayerIndices.resize(id.first+1);
            if (static_cast<short>(mLayerIndices[id.first].size()) < id.second+1)
                mLayerIndices[id.first].resize(id.second+1);
            mLayerIndices[id.first][id.second] = index;
        }
        return layerInfos;
    }

    vsg::ref_ptr<vsg::Data> Storage::getBlendmap(const Terrain::Bounds& bounds)
    {
        int cellX = static_cast<int>(std::floor(bounds.min.x));
        int cellY = static_cast<int>(std::floor(bounds.min.y));

        int rowStart = (bounds.min.x - cellX) * ESM::Land::LAND_TEXTURE_SIZE;
        int colStart = (bounds.min.y - cellY) * ESM::Land::LAND_TEXTURE_SIZE;

        const int blendmapSize = ESM::Land::LAND_TEXTURE_SIZE * bounds.size;

        LandCache cache;

        auto image = vsg::ubvec4Array2D::create(blendmapSize, blendmapSize, vsg::ubvec4(), vsg::Data::Properties{ VK_FORMAT_R8G8B8A8_UNORM }); // _UINT
        for (int x = 0; x < blendmapSize; x++)
        {
            for (int y = 0; y < blendmapSize; y++)
            {
                for (int i = 0; i < 4; ++i)
                {
                    vsg::ivec2 offset = { i&1, (i&2)>>1 };
                    PluginTexture id = getVtexIndexAt(cellX, cellY, x + rowStart + offset.x, y + colStart + offset.y, cache);
                    unsigned int layerIndex = 0;
                    if (/*id.first < static_cast<short>(mLayerIndices.size()) && */id.second < static_cast<short>(mLayerIndices[id.first].size()))
                        layerIndex = mLayerIndices[id.first][id.second];
                    (*image)(x, y)[i] = static_cast<unsigned char>(layerIndex);
                }
            }
        }
        return image;
    }

    float Storage::getHeightAt(const vsg::vec3& worldPos, ESM::RefId worldspace)
    {
        int cellX = static_cast<int>(std::floor(worldPos.x / cellWorldSize));
        int cellY = static_cast<int>(std::floor(worldPos.y / cellWorldSize));
        auto land = getLand({ cellX, cellY, worldspace });
        if (!land)
            return defaultHeight;

        const ESM::Land::LandData* data = land->getData(ESM::Land::DATA_VHGT);
        if (!data)
            return defaultHeight;

        // Mostly lifted from Ogre::Terrain::getHeightAtTerrainPosition

        // Normalized position in the cell
        float nX = (worldPos.x - (cellX * cellWorldSize)) / cellWorldSize;
        float nY = (worldPos.y - (cellY * cellWorldSize)) / cellWorldSize;

        // get left / bottom points (rounded down)
        float factor = ESM::Land::LAND_SIZE - 1.0f;
        float invFactor = 1.0f / factor;

        int startX = static_cast<int>(nX * factor);
        int startY = static_cast<int>(nY * factor);
        int endX = startX + 1;
        int endY = startY + 1;

        endX = std::min(endX, ESM::Land::LAND_SIZE - 1);
        endY = std::min(endY, ESM::Land::LAND_SIZE - 1);

        // now get points in terrain space (effectively rounding them to boundaries)
        float startXTS = startX * invFactor;
        float startYTS = startY * invFactor;
        float endXTS = endX * invFactor;
        float endYTS = endY * invFactor;

        // get parametric from start coord to next point
        float xParam = (nX - startXTS) * factor;
        float yParam = (nY - startYTS) * factor;

        /* For even / odd tri strip rows, triangles are this shape:
        even     odd
        3---2   3---2
        | / |   | \ |
        0---1   0---1
        */

        // Build all 4 positions in normalized cell space, using point-sampled height
        vsg::vec3 v0(startXTS, startYTS, getVertexHeight(data, startX, startY) / cellWorldSize);
        vsg::vec3 v1(endXTS, startYTS, getVertexHeight(data, endX, startY) / cellWorldSize);
        vsg::vec3 v2(endXTS, endYTS, getVertexHeight(data, endX, endY) / cellWorldSize);
        vsg::vec3 v3(startXTS, endYTS, getVertexHeight(data, startX, endY) / cellWorldSize);
        // define this plane in terrain space
        osg::Plane plane;
        // FIXME: deal with differing triangle alignment
        if (true)
        {
            // odd row
            bool secondTri = ((1.0 - yParam) > xParam);
            if (secondTri)
                plane = osg::Plane(toOsg(v0), toOsg(v1), toOsg(v3));
            else
                plane = osg::Plane(toOsg(v1), toOsg(v2), toOsg(v3));
        }
        /*
        else
        {
            // even row
            bool secondTri = (yParam > xParam);
            if (secondTri)
                plane.redefine(v0, v2, v3);
            else
                plane.redefine(v0, v1, v2);
        }
        */

        // Solve plane equation for z
        return (-plane.getNormal().x() * nX - plane.getNormal().y() * nY - plane[3]) / plane.getNormal().z() * cellWorldSize;
    }

    const LandObject* Storage::getLand(int cellX, int cellY, LandCache& cache)
    {
        LandCache::Map::iterator found = cache.mMap.find(std::make_pair(cellX, cellY));
        if (found != cache.mMap.end())
            return found->second;
        else
        {
            found = cache.mMap.insert(std::make_pair(std::make_pair(cellX, cellY), getLand({ cellX, cellY, {} }))).first;
            return found->second;
        }
    }
}
