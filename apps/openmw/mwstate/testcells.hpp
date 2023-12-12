#ifndef VSGOPENMW_MWSTATE_TESTCELLS_H
#define VSGOPENMW_MWSTATE_TESTCELLS_H

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwworld/scene.hpp"

#include "loadingscreen.hpp"

namespace MWState
{
    class TestCells : public ThreadLoading
    {
    public:
        bool interior{};
        void threadLoad() override
        {
            auto& scene = *MWBase::Environment::get().getWorldScene();
            if (interior)
                scene.testInteriorCells(*this);
            else
                scene.testExteriorCells(*this);
        }

        static std::shared_ptr<GameState> makeLoadingScreen(bool interior)
        {
            auto loading = std::make_shared<TestCells>();
            loading->interior = interior;
            auto loadingScreen = MWBase::Environment::get().getWindowManager()->createLoadingScreen();
            loadingScreen->loading = loading;
            loadingScreen->setWallpaper(true);
            return loadingScreen;
        }
    };
}

#endif
