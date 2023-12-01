// vsgopenmw-unity-build

#include <functional>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwworld/worldmodel.hpp"

#include <components/esm/refid.hpp>

#include "preloadcell.hpp"
#include "operation.hpp"
#include "loadingscreen.hpp"

namespace MWState
{
    class ChangeCell : public Operation
    {
        MWWorld::Scene& mScene;
    public:
        ChangeCell(MWWorld::Scene& scene)
            : mScene(scene)
        {
        }
        ESM::Position position;
        ESM::RefId cell;
        bool changeEvent{};
        std::function<void(const MWWorld::CellStore&)> onCellChanged;

        bool useFading(const MWWorld::CellStore& cellStore) const
        {
            return (cellStore.isExterior() && changeEvent) || (!cellStore.isExterior() && mScene.getCurrentCell() != nullptr);
        }

        void pushPreloadState(MWBase::StateManager& stateMgr)
        {
            auto& cellStore = MWBase::Environment::get().getWorldModel()->getCell(cell);
            if (mScene.preloadCell(cellStore, true) < 1)
            {
                auto loadingScreen = MWBase::Environment::get().getWindowManager()->createLoadingScreen();
                loadingScreen->loading = std::make_shared<PreloadCell>(mScene, cell);
                loadingScreen->setWallpaper(mScene.getCurrentCell() == nullptr);
                stateMgr.pushGameState(loadingScreen);
                if (useFading(cellStore))
                    MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);
            }
        }

        bool run(float dt)
        {
            if (MWBase::Environment::get().getStateManager()->hasQuitRequest())
                return false;

            auto& cellStore = MWBase::Environment::get().getWorldModel()->getCell(cell);
            if (cellStore.isExterior())
                mScene.changeToExteriorCell(cell, position, changeEvent);
            else
                mScene.changeToInteriorCell(cellStore.getCell()->getNameId(), position, changeEvent);

            if (onCellChanged)
                onCellChanged(cellStore);

            if (useFading(cellStore))
                MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);
            return false;
        }
    };
}
