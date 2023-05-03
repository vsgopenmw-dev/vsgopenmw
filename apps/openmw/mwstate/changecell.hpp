// vsgopenmw-unity-build

#include <functional>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/statemanager.hpp"

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
        ESM::CellId cell;
        bool changeEvent{};
        std::function<void(const ESM::CellId&)> onCellChanged;

        bool useFading() const
        {
            return (cell.mPaged && changeEvent) || (!cell.mPaged && mScene.getCurrentCell() != nullptr);
        }

        void pushPreloadState(MWBase::StateManager& stateMgr)
        {
            if (mScene.preloadCell(cell, true) < 1)
            {
                auto loadingScreen = MWBase::Environment::get().getWindowManager()->createLoadingScreen();
                loadingScreen->loading = std::make_shared<PreloadCell>(mScene, cell);
                loadingScreen->setWallpaper(mScene.getCurrentCell() == nullptr);
                stateMgr.pushGameState(loadingScreen);
                if (useFading())
                    MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);
            }
        }

        bool run(float dt)
        {
            if (MWBase::Environment::get().getStateManager()->hasQuitRequest())
                return false;

            if (cell.mPaged)
                mScene.changeToExteriorCell(position, changeEvent);
            else
                mScene.changeToInteriorCell(cell.mWorldspace, position, changeEvent);

            if (onCellChanged)
                onCellChanged(cell);

            if (useFading())
                MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);
            return false;
        }
    };
}
