// vsgopenmw-unity-build

#include <memory>

#include <components/fallback/fallback.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/l10n/manager.hpp>

#include "mwbase/soundmanager.hpp"
#include "mwstate/executescript.hpp"
#include "mwstate/loading.hpp"
#include "mwstate/menu.hpp"

#include "game.hpp"

namespace OMW
{
    /*
     * Creates game.
     */
    class Boot : public MWState::ThreadLoading
    {
        Engine& mEngine;
        std::unique_ptr<MWWorld::World> mWorld;
        MWBase::Environment& mEnvironment;
        const Arguments mArgs;

        bool mPlayedLogo = false;

        void threadLoad() override
        {
            MWClass::registerClasses();
            mWorld
                = std::make_unique<MWWorld::World>(*mEngine.mRenderManager, mEngine.mResourceSystem.get(),
                    mEngine.mFileCollections, mArgs.contentFiles, /*mArgs.groundcoverFiles*/ std::vector<std::string>(),
                    mEngine.mEncoder.get(), *this, mArgs.activationDistanceOverride,
                    mEngine.mCfgMgr.getUserDataPath());
            mEnvironment.setWorld(*mWorld);
            mEnvironment.setWorldModel(mWorld->getWorldModel());
            mEnvironment.setWorldScene(mWorld->getWorldScene());

            const MWWorld::Store<ESM::GameSetting>& gmst = mEnvironment.getWorld()->getStore().get<ESM::GameSetting>();
            mEngine.mL10nManager->setGmstLoader([gmst](std::string_view gmstName) {
                const ESM::GameSetting* res = gmst.search(gmstName);
                if (res && res->mValue.getType() == ESM::VT_String)
                    return res->mValue.getString();
                else
                {
                    std::cerr << "GMST " << gmstName << " not found" << std::endl;
                    return std::string("GMST:") + std::string(gmstName);
                }
            });
        }
        bool run(float dt) override
        {
            // playLogoWhileLoadingWorld
            bool threadRunning = ThreadLoading::run(dt);
            if (!mPlayedLogo && !mArgs.skipMenu)
            {
                auto logo = Fallback::Map::getString("Movies_Company_Logo");
                if (!logo.empty())
                    mEnvironment.getWindowManager()->playVideo(logo, true);
                mPlayedLogo = true;
                return true;
            }
            if (threadRunning)
                return true;
            else if (abort)
                return false;

            // initializeWorldDependentObjects
            mEngine.mStateManager->pushGameState(std::make_shared<Game>(mEngine, mWorld, mArgs));

            if (!mArgs.startupScript.empty())
                mEnvironment.getStateManager()->pushGameState(std::make_shared<MWState::ExecuteScript>(mArgs.startupScript));

            if (!mArgs.saveGameFile.empty())
                mEngine.mStateManager->loadGame(mArgs.saveGameFile);
            else if (!mArgs.skipMenu)
            {
                mEngine.mStateManager->mainMenu();
                mEnvironment.getSoundManager()->playPlaylist("Title");
                auto logo = Fallback::Map::getString("Movies_Morrowind_Logo");
                if (!logo.empty())
                    mEnvironment.getWindowManager()->playVideo(logo, true, false);
            }
            else if (mArgs.newGame)
                mEngine.mStateManager->newGame();
            else
                mEngine.mStateManager->bypassNewGame(mArgs.cellName);
            return false;
        }
    public:
        Boot(Engine& engine, const Arguments& args)
            : mEngine(engine)
            , mEnvironment(mEngine.mEnvironment)
            , mArgs(args)
        {
        }
    };
}
