#include "storage.hpp"
#include <set>

#include <vsg/core/Array3D.h>

#include <osg/Plane>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/vfs/manager.hpp>
#include <components/vsgadapters/osgcompat.hpp>

namespace ESMTerrain
{
    class LandCache
    {
    public:
        using Map = std::map<std::pair<int, int>, vsg::ref_ptr<const LandObject>>;
        Map mMap;
    };

    LandObject::LandObject(const ESM::Land* land, int loadFlags)
        : mLand(land)
        , mLoadFlags(loadFlags)
    {
        mLand->loadData(mLoadFlags, &mData);
    }

    LandObject::~LandObject() {}

    const float defaultHeight = ESM::Land::DEFAULT_HEIGHT;

    Storage::Storage(const VFS::Manager* vfs, const std::string& normalMapPattern,
        const std::string& normalHeightMapPattern, bool autoUseNormalMaps, const std::string& specularMapPattern,
        bool autoUseSpecularMaps)
        : Terrain::Storage(static_cast<float>(ESM::Land::REAL_SIZE), static_cast<unsigned int>(ESM::Land::LAND_SIZE))
        , mVFS(vfs)
        , mNormalMapPattern(normalMapPattern)
        , mNormalHeightMapPattern(normalHeightMapPattern)
        , mAutoUseNormalMaps(autoUseNormalMaps)
        , mSpecularMapPattern(specularMapPattern)
        , mAutoUseSpecularMaps(autoUseSpecularMaps)
    {
    }

    bool Storage::getMinMaxHeights(float size, const vsg::vec2& center, float& min, float& max)
    {
        assert(size <= 1 && "Storage::getMinMaxHeights, chunk size should be <= 1 cell");

        vsg::vec2 origin = center - vsg::vec2(size / 2.f, size / 2.f);

        int cellX = static_cast<int>(std::floor(origin.x));
        int cellY = static_cast<int>(std::floor(origin.y));

        int startRow = (origin.x - cellX) * ESM::Land::LAND_SIZE;
        int startColumn = (origin.y - cellY) * ESM::Land::LAND_SIZE;

        int endRow = startRow + size * (ESM::Land::LAND_SIZE - 1) + 1;
        int endColumn = startColumn + size * (ESM::Land::LAND_SIZE - 1) + 1;

        auto land = getLand(cellX, cellY);
        const ESM::Land::LandData* data = land ? land->getData(ESM::Land::DATA_VHGT) : nullptr;
        if (data)
        {
            min = std::numeric_limits<float>::max();
            max = -std::numeric_limits<float>::max();
            for (int row = startRow; row < endRow; ++row)
            {
                for (int col = startColumn; col < endColumn; ++col)
                {
                    float h = data->mHeights[col * ESM::Land::LAND_SIZE + row];
                    if (h > max)
                        max = h;
                    if (h < min)
                        min = h;
                }
            }
            return true;
        }

        min = defaultHeight;
        max = defaultHeight;
        return false;
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

    void Storage::fillVertexBuffers(int lodLevel, float size, const vsg::vec2& center,
        vsg::ref_ptr<vsg::floatArray>& heights, vsg::ref_ptr<vsg::vec3Array>& normals,
        vsg::ref_ptr<vsg::ubvec4Array>& colours)
    {
        // LOD level n means every 2^n-th vertex is kept
        size_t increment = static_cast<size_t>(1) << lodLevel;

        vsg::vec2 origin = center - vsg::vec2(size / 2.f, size / 2.f);

        int startCellX = static_cast<int>(std::floor(origin.x));
        int startCellY = static_cast<int>(std::floor(origin.y));

        size_t numVerts = static_cast<size_t>(size * (ESM::Land::LAND_SIZE - 1) / increment + 1);

        heights = vsg::floatArray::create(numVerts * numVerts);
        normals = vsg::vec3Array::create(numVerts * numVerts);
        colours = vsg::ubvec4Array::create(numVerts * numVerts);

        vsg::vec3 normal;
        vsg::ubvec4 color;

        float vertY = 0;
        float vertX = 0;

        LandCache cache;

        //bool alteration = useAlteration();

        float vertY_ = 0; // of current cell corner
        for (int cellY = startCellY; cellY < startCellY + std::ceil(size); ++cellY)
        {
            float vertX_ = 0; // of current cell corner
            for (int cellX = startCellX; cellX < startCellX + std::ceil(size); ++cellX)
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
                rowStart += (origin.x - startCellX) * ESM::Land::LAND_SIZE;
                colStart += (origin.y - startCellY) * ESM::Land::LAND_SIZE;
                int rowEnd = std::min(static_cast<int>(rowStart + std::min(1.f, size) * (ESM::Land::LAND_SIZE - 1) + 1),
                    static_cast<int>(ESM::Land::LAND_SIZE));
                int colEnd = std::min(static_cast<int>(colStart + std::min(1.f, size) * (ESM::Land::LAND_SIZE - 1) + 1),
                    static_cast<int>(ESM::Land::LAND_SIZE));

                vertY = vertY_;
                for (int col = colStart; col < colEnd; col += increment)
                {
                    vertX = vertX_;
                    for (int row = rowStart; row < rowEnd; row += increment)
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
                        (*heights)[static_cast<unsigned int>(vertX * numVerts + vertY)] = height;
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

                        assert(normal.z() > 0);

                        (*normals)[static_cast<unsigned int>(vertX * numVerts + vertY)] = normal;

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

                        (*colours)[static_cast<unsigned int>(vertX * numVerts + vertY)] = color;

                        ++vertX;
                    }
                    ++vertY;
                }
                vertX_ = vertX;
            }
            vertY_ = vertY;

            assert(vertX_ == numVerts); // Ensure we covered whole area
        }
        assert(vertY_ == numVerts); // Ensure we covered whole area
    }

    Storage::UniqueTextureId Storage::getVtexIndexAt(int cellX, int cellY, int x, int y, LandCache& cache)
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
            if (tex == 0)
                return std::make_pair(0, 0); // vtex 0 is always the base texture, regardless of plugin
            return std::make_pair(tex, land->getPlugin());
        }
        return std::make_pair(0, 0);
    }

    std::string Storage::getTextureName(UniqueTextureId id)
    {
        static constexpr char defaultTexture[] = "_land_default.dds";
        if (id.first == 0)
            return defaultTexture; // Not sure if the default texture really is hardcoded?

        // NB: All vtex ids are +1 compared to the ltex ids
        const ESM::LandTexture* ltex = getLandTexture(id.first - 1, id.second);
        if (!ltex)
        {
            Log(Debug::Warning) << "Warning: Unable to find land texture index " << id.first - 1 << " in plugin "
                                << id.second << ", using default texture instead";
            return defaultTexture;
        }

        return ltex->mTexture;
    }

    void Storage::getBlendmaps(
        float chunkSize, const vsg::vec2& chunkCenter, vsg::ref_ptr<vsg::Data>& blendmaps, std::vector<Terrain::LayerInfo>& layerList)
    {
        vsg::vec2 origin = chunkCenter - vsg::vec2(chunkSize / 2.f, chunkSize / 2.f);
        int cellX = static_cast<int>(std::floor(origin.x));
        int cellY = static_cast<int>(std::floor(origin.y));

        int realTextureSize = ESM::Land::LAND_TEXTURE_SIZE + 1; // add 1 to wrap around next cell

        int rowStart = (origin.x - cellX) * realTextureSize;
        int colStart = (origin.y - cellY) * realTextureSize;

        const int blendmapSize = (realTextureSize - 1) * chunkSize + 1;
        // vsgopenmw-fixme
        // We need to upscale the blendmap 2x with nearest neighbor sampling to look like Vanilla
        const int imageScaleFactor = 2;
        const int blendmapImageSize = blendmapSize * imageScaleFactor;

        LandCache cache;
        std::map<UniqueTextureId, unsigned int> textureIndicesMap;

        for (int y = 0; y < blendmapSize; y++)
        {
            for (int x = 0; x < blendmapSize; x++)
            {
                UniqueTextureId id = getVtexIndexAt(cellX, cellY, x + rowStart, y + colStart, cache);
                std::map<UniqueTextureId, unsigned int>::iterator found = textureIndicesMap.find(id);
                if (found == textureIndicesMap.end())
                {
                    unsigned int layerIndex = layerList.size();
                    Terrain::LayerInfo info = getLayerInfo(getTextureName(id));

                    // look for existing diffuse map, which may be present when several plugins use the same texture
                    for (unsigned int i = 0; i < layerList.size(); ++i)
                    {
                        if (layerList[i].mDiffuseMap == info.mDiffuseMap)
                        {
                            layerIndex = i;
                            break;
                        }
                    }

                    textureIndicesMap.emplace(id, layerIndex);

                    if (layerIndex >= layerList.size())
                        layerList.emplace_back(info);
                }
            }
        }

        auto image = vsg::ubyteArray3D::create(blendmapImageSize, blendmapImageSize, layerList.size(), static_cast<vsg::ubyteArray3D::value_type>(0), vsg::Data::Properties{ VK_FORMAT_R8_UNORM });
        for (int x = 0; x < blendmapSize; x++)
        {
            for (int y = 0; y < blendmapSize; y++)
            {
                UniqueTextureId id = getVtexIndexAt(cellX, cellY, x + rowStart, y + colStart, cache);
                int layerIndex = textureIndicesMap[id];
                // vsgopenmw-fixme
                // int realY = y;
                int realY = y * imageScaleFactor;
                int realX = x * imageScaleFactor;
                (*image)(realX, realY, layerIndex) =
                    (*image)(realX + 1, realY, layerIndex) =
                    (*image)(realX, realY + 1 , layerIndex) =
                    (*image)(realX + 1, realY + 1, layerIndex) = 255;
            }
        }
        blendmaps = image;
    }

    float Storage::getHeightAt(const vsg::vec3& worldPos)
    {
        int cellX = static_cast<int>(std::floor(worldPos.x / float(Constants::CellSizeInUnits)));
        int cellY = static_cast<int>(std::floor(worldPos.y / float(Constants::CellSizeInUnits)));

        auto land = getLand(cellX, cellY);
        if (!land)
            return defaultHeight;

        const ESM::Land::LandData* data = land->getData(ESM::Land::DATA_VHGT);
        if (!data)
            return defaultHeight;

        // Mostly lifted from Ogre::Terrain::getHeightAtTerrainPosition

        // Normalized position in the cell
        float nX = (worldPos.x - (cellX * Constants::CellSizeInUnits)) / float(Constants::CellSizeInUnits);
        float nY = (worldPos.y - (cellY * Constants::CellSizeInUnits)) / float(Constants::CellSizeInUnits);

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
        vsg::vec3 v0(startXTS, startYTS, getVertexHeight(data, startX, startY) / float(Constants::CellSizeInUnits));
        vsg::vec3 v1(endXTS, startYTS, getVertexHeight(data, endX, startY) / float(Constants::CellSizeInUnits));
        vsg::vec3 v2(endXTS, endYTS, getVertexHeight(data, endX, endY) / float(Constants::CellSizeInUnits));
        vsg::vec3 v3(startXTS, endYTS, getVertexHeight(data, startX, endY) / float(Constants::CellSizeInUnits));
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
        return (-plane.getNormal().x() * nX - plane.getNormal().y() * nY - plane[3]) / plane.getNormal().z()
            * Constants::CellSizeInUnits;
    }

    const LandObject* Storage::getLand(int cellX, int cellY, LandCache& cache)
    {
        LandCache::Map::iterator found = cache.mMap.find(std::make_pair(cellX, cellY));
        if (found != cache.mMap.end())
            return found->second;
        else
        {
            found = cache.mMap.insert(std::make_pair(std::make_pair(cellX, cellY), getLand(cellX, cellY))).first;
            return found->second;
        }
    }

    void Storage::adjustColor(int col, int row, const ESM::Land::LandData* heightData, vsg::ubvec4& color) const {}

    float Storage::getAlteredHeight(int col, int row) const
    {
        return 0;
    }

    Terrain::LayerInfo Storage::getLayerInfo(const std::string& texture)
    {
        std::lock_guard<std::mutex> lock(mLayerInfoMutex);

        // Already have this cached?
        std::map<std::string, Terrain::LayerInfo>::iterator found = mLayerInfoMap.find(texture);
        if (found != mLayerInfoMap.end())
            return found->second;

        Terrain::LayerInfo info;
        info.mParallax = false;
        info.mSpecular = false;
        info.mDiffuseMap = texture;

        if (mAutoUseNormalMaps)
        {
            std::string texture_ = texture;
            Misc::StringUtils::replaceLast(texture_, ".", mNormalHeightMapPattern + ".");
            if (mVFS->exists(texture_))
            {
                info.mNormalMap = texture_;
                info.mParallax = true;
            }
            else
            {
                texture_ = texture;
                Misc::StringUtils::replaceLast(texture_, ".", mNormalMapPattern + ".");
                if (mVFS->exists(texture_))
                    info.mNormalMap = texture_;
            }
        }

        if (mAutoUseSpecularMaps)
        {
            std::string texture_ = texture;
            Misc::StringUtils::replaceLast(texture_, ".", mSpecularMapPattern + ".");
            if (mVFS->exists(texture_))
            {
                info.mDiffuseMap = texture_;
                info.mSpecular = true;
            }
        }

        mLayerInfoMap[texture] = info;
        return info;
    }
}
