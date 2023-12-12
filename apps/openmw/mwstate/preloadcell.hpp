// vsgopenmw-unity-build

#include <components/esm/refid.hpp>

#include "../mwworld/scene.hpp"
#include "../mwworld/worldmodel.hpp"
#include "../mwbase/environment.hpp"

#include "loading.hpp"

namespace MWState
{
    class PreloadCell : public Loading
    {
        MWWorld::Scene& mScene;
    public:
        ESM::RefId cell;
        PreloadCell(MWWorld::Scene& scene, const ESM::RefId& in_cell)
            : mScene(scene)
            , cell(in_cell)
        {
        }
        bool run(float dt) override
        {
            auto& cellStore = MWBase::Environment::get().getWorldModel()->getCell(cell);
            if (cellStore.isExterior())
                mDescription = "#{OMWEngine:LoadingExterior}";
            else
                mDescription = "#{OMWEngine:LoadingInterior}";

            mComplete = mScene.preloadCell(cellStore, true);
            return mComplete < 1 && !abort;
        }
    };
}
