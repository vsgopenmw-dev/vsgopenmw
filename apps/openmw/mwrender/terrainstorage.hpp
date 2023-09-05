#ifndef MWRENDER_TERRAINSTORAGE_H
#define MWRENDER_TERRAINSTORAGE_H

#include <memory>

#include <components/esm3terrain/storage.hpp>

#include <components/resource/resourcesystem.hpp>

namespace MWWorld
{
    class ESMStore;
}
namespace MWRender
{

    class LandManager;

    /// @brief Connects the ESM Store used in OpenMW with the ESMTerrain storage.
    class TerrainStorage : public ESMTerrain::Storage
    {
    public:
        TerrainStorage(Resource::ResourceSystem* resourceSystem);
        ~TerrainStorage();

        void setStore(const MWWorld::ESMStore* store) { mEsmStore = store; }

        vsg::ref_ptr<const ESMTerrain::LandObject> getLand(int cellX, int cellY) const override;
        std::map<PluginTexture, std::string> enumerateLayers() const override;

        LandManager* getLandManager() const;

    private:
        const MWWorld::ESMStore* mEsmStore =nullptr;
        std::unique_ptr<LandManager> mLandManager;

        Resource::ResourceSystem* mResourceSystem;
    };

}

#endif
