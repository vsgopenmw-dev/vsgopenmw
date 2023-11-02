#include "terrainstorage.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/manager.hpp>

#include "landmanager.hpp"

namespace MWRender
{
    TerrainStorage::TerrainStorage(Resource::ResourceSystem* resourceSystem)
        : mLandManager(new LandManager(
              ESM::Land::DATA_VCLR | ESM::Land::DATA_VHGT | ESM::Land::DATA_VNML | ESM::Land::DATA_VTEX))
        , mResourceSystem(resourceSystem)
    {
    }

    TerrainStorage::~TerrainStorage()
    {
    }

    LandManager* TerrainStorage::getLandManager() const
    {
        return mLandManager.get();
    }

    vsg::ref_ptr<const ESMTerrain::LandObject> TerrainStorage::getLand(int cellX, int cellY) const
    {
        return mLandManager->getLand(cellX, cellY);
    }

    std::map<TerrainStorage::PluginTexture, std::string> TerrainStorage::enumerateLayers() const
    {
        std::map<PluginTexture, std::string> map;
        const auto& store = mEsmStore->get<ESM::LandTexture>();
        static constexpr char defaultTexture[] = "_land_default.dds";
        for (size_t plugin = 0; plugin < store.getSize(); ++plugin)
        {
            map[std::make_pair(0, plugin)] = defaultTexture; // Not sure if the default texture really is hardcoded?
            for (size_t texture = 0; texture < store.getSize(plugin); ++texture)
            {
                auto lt = store.search(texture, plugin);
                if (lt && mResourceSystem->getVFS()->exists(Misc::ResourceHelpers::correctTexturePath(lt->mTexture, mResourceSystem->getVFS())))
                {
                    // NB: All vtex ids are +1 compared to the ltex ids
                    map[std::make_pair(texture+1, plugin)] = lt->mTexture;
                }
            }
        }
        return map;
    }
}
