#ifndef COMPONENTS_ESM_TERRAIN_STORAGE_H
#define COMPONENTS_ESM_TERRAIN_STORAGE_H

#include <cassert>
#include <mutex>

#include <components/terrain/storage.hpp>

#include <components/esm3/loadland.hpp>
#include <components/esm3/loadltex.hpp>

namespace VFS
{
    class Manager;
}

namespace ESMTerrain
{

    class LandCache;

    /// @brief Wrapper around Land Data with reference counting. The wrapper needs to be held as long as the data is
    /// still in use
    class LandObject : public vsg::Object
    {
    public:
        LandObject(const ESM::Land* land, int loadFlags);
        virtual ~LandObject();

        inline const ESM::Land::LandData* getData(int flags) const
        {
            if ((mData.mDataLoaded & flags) != flags)
                return nullptr;
            return &mData;
        }
        inline int getPlugin() const { return mLand->getPlugin(); }

    private:
        const ESM::Land* mLand{};
        int mLoadFlags{};

        ESM::Land::LandData mData;
    };

    /// @brief Feeds data from ESM terrain records (ESM::Land, ESM::LandTexture)
    ///        into the terrain component, converting it on the fly as needed.
    class Storage : public Terrain::Storage
    {
    public:
        Storage(const VFS::Manager* vfs, const std::string& normalMapPattern = "",
            const std::string& normalHeightMapPattern = "", bool autoUseNormalMaps = false,
            const std::string& specularMapPattern = "", bool autoUseSpecularMaps = false);

        // Not implemented in this class, because we need different Store implementations for game and editor
        virtual vsg::ref_ptr<const LandObject> getLand(int cellX, int cellY) = 0;
        virtual const ESM::LandTexture* getLandTexture(int index, short plugin) = 0;
        /// Get bounds of the whole terrain in cell units
        void getBounds(float& minX, float& maxX, float& minY, float& maxY) override = 0;

        /// Get the minimum and maximum heights of a terrain region.
        /// @note Will only be called for chunks with size = minBatchSize, i.e. leafs of the quad tree.
        ///        Larger chunks can simply merge AABB of children.
        /// @param size size of the chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param min min height will be stored here
        /// @param max max height will be stored here
        /// @return true if there was data available for this terrain chunk
        bool getMinMaxHeights(float size, const vsg::vec2& center, float& min, float& max) override;

        /// Fill vertex buffers for a terrain chunk.
        /// @note May be called from background threads. Make sure to only call thread-safe functions from here!
        /// @note Vertices should be written in row-major order (a row is defined as parallel to the x-axis).
        ///       The specified positions should be in local space, i.e. relative to the center of the terrain chunk.
        /// @param lodLevel LOD level, 0 = most detailed
        /// @param size size of the terrain chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param positions buffer to write vertices
        /// @param normals buffer to write vertex normals
        /// @param colours buffer to write vertex colours
        void fillVertexBuffers(int lodLevel, float size, const vsg::vec2& center,
            vsg::ref_ptr<vsg::floatArray>& out_heights, vsg::ref_ptr<vsg::vec3Array>& out_normals,
            vsg::ref_ptr<vsg::ubvec4Array>& out_colours) override;

        /// Create textures holding layer blend values for a terrain chunk.
        /// @note The terrain chunk shouldn't be larger than one cell since otherwise we might
        ///       have to do a ridiculous amount of different layers. For larger chunks, composite maps should be used.
        /// @note May be called from background threads.
        /// @param chunkSize size of the terrain chunk in cell units
        /// @param chunkCenter center of the chunk in cell units
        /// @param blendmaps created blendmaps will be written here
        /// @param layerList names of the layer textures used will be written here
        void getBlendmaps(float chunkSize, const vsg::vec2& chunkCenter, vsg::ref_ptr<vsg::Data>& blendmaps,
            std::vector<Terrain::LayerInfo>& layerList) override;

        float getHeightAt(const vsg::vec3& worldPos) override;

        float getVertexHeight(const ESM::Land::LandData* data, int x, int y)
        {
            assert(x < ESM::Land::LAND_SIZE);
            assert(y < ESM::Land::LAND_SIZE);
            return data->mHeights[y * ESM::Land::LAND_SIZE + x];
        }

    private:
        const VFS::Manager* mVFS;

        inline void fixNormal(vsg::vec3& normal, int cellX, int cellY, int col, int row, LandCache& cache);
        inline void fixColour(vsg::ubvec4& colour, int cellX, int cellY, int col, int row, LandCache& cache);
        inline void averageNormal(vsg::vec3& normal, int cellX, int cellY, int col, int row, LandCache& cache);

        inline const LandObject* getLand(int cellX, int cellY, LandCache& cache);

        virtual bool useAlteration() const { return false; }
        virtual void adjustColor(int col, int row, const ESM::Land::LandData* heightData, vsg::ubvec4& color) const;
        virtual float getAlteredHeight(int col, int row) const;

        // Since plugins can define new texture palettes, we need to know the plugin index too
        // in order to retrieve the correct texture name.
        // pair  <texture id, plugin id>
        typedef std::pair<short, short> UniqueTextureId;

        inline UniqueTextureId getVtexIndexAt(int cellX, int cellY, int x, int y, LandCache&);
        std::string getTextureName(UniqueTextureId id);

        std::map<std::string, Terrain::LayerInfo> mLayerInfoMap;
        std::mutex mLayerInfoMutex;

        std::string mNormalMapPattern;
        std::string mNormalHeightMapPattern;
        bool mAutoUseNormalMaps;

        std::string mSpecularMapPattern;
        bool mAutoUseSpecularMaps;

        Terrain::LayerInfo getLayerInfo(const std::string& texture);
    };

}

#endif
