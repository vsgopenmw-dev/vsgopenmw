#ifndef OPENMW_COMPONENTS_ESMTERRAIN_STORAGE_H
#define OPENMW_COMPONENTS_ESMTERRAIN_STORAGE_H

#include <components/terrain/storage.hpp>

#include <components/esm/util.hpp>
#include <components/esm3/loadltex.hpp>
#include <components/esm3/loadland.hpp>

namespace ESMTerrain
{
    class LandCache;

    /// @brief Wrapper around Land Data with reference counting. The wrapper needs to be held as long as the data is
    /// still in use
    class LandObject : public vsg::Object
    {
    public:
        LandObject() = default; // Note, a default constructed LandObject indicates there is no terrain at the specified cell coordinates.
        LandObject(const ESM::Land& land, int loadFlags);
        virtual ~LandObject();

        inline const ESM::Land::LandData* getData(int flags) const
        {
            if ((mData.mDataLoaded & flags) != flags)
                return nullptr;

            return &mData;
        }

        int getPlugin() const { return mLand->getPlugin(); }

    private:
        const ESM::Land* mLand{};
        int mLoadFlags{};
        ESM::Land::LandData mData;
    };

    // Since plugins can define new texture palettes, we need to know the plugin index too
    // in order to retrieve the correct texture name.
    // pair  <texture id, plugin id>
    using UniqueTextureId = std::pair<std::uint16_t, int>;

    /// @brief Feeds data from ESM terrain records (ESM::Land, ESM::LandTexture)
    ///        into the terrain component, converting it on the fly as needed.
    /// TODO: shouldn't all methods be made const?
    class Storage : public Terrain::Storage
    {
    public:
        Storage();

        // Since plugins can define new texture palettes, we need to know the plugin index too
        // in order to retrieve the correct texture name.
        // pair  <texture id, plugin id>
        using PluginTexture = std::pair<short, short>;

        // Not implemented in this class, because we need different Store implementations for game and editor
        // TODO: wouldn't it be more efficient to pass worldspaces elsewhere? create a Storage or View object per worldspace?
        virtual vsg::ref_ptr<const LandObject> getLand(ESM::ExteriorCellLocation cellLocation) = 0;
        virtual std::map<PluginTexture, std::string> enumerateLayers() const = 0;

        /// Fill vertex buffers for a terrain chunk.
        /// @param lodLevel LOD level, 0 = most detailed
        VertexData getVertexData(int lodLevel, const Terrain::Bounds& bounds) override;

        /// Create textures holding layer blend values for a terrain chunk.
        vsg::ref_ptr<vsg::Data> getBlendmap(const Terrain::Bounds& bounds) override;

        std::vector<Terrain::LayerInfo> getLayers() override;

        float getHeightAt(const vsg::vec3& worldPos, ESM::RefId worldspace) override;

        float getVertexHeight(const ESM::Land::LandData* data, int x, int y) const
        {
            return data->mHeights[y * ESM::Land::LAND_SIZE + x];
        }

    private:
        inline void fixNormal(vsg::vec3& normal, int cellX, int cellY, int col, int row, LandCache& cache);
        inline void fixColour(vsg::ubvec4& colour, int cellX, int cellY, int col, int row, LandCache& cache);
        inline void averageNormal(vsg::vec3& normal, int cellX, int cellY, int col, int row, LandCache& cache);

        inline const LandObject* getLand(int cellX, int cellY, LandCache& cache);

        virtual bool useAlteration() const { return false; }
        virtual void adjustColor(int col, int row, const ESM::Land::LandData* heightData, vsg::ubvec4& color) const {}
        virtual float getAlteredHeight(int col, int row) const { return 0.f; }

        inline PluginTexture getVtexIndexAt(int cellX, int cellY, int x, int y, LandCache&);

        std::vector<std::vector<unsigned int>> mLayerIndices;
    };

}

#endif
