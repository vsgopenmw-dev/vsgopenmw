//vsgopenmw-unity-build

#include "game.hpp"

#include "mwbase/environment.hpp"
#include "mwbase/scriptmanager.hpp"
#include "mwbase/soundmanager.hpp"
#include "mwbase/statemanager.hpp"
#include "mwdialogue/scripttest.hpp"
#include "mwlua/luamanagerimp.hpp"
#include "mwscript/scriptmanagerimp.hpp"
#include "mwgui/windowmanagerimp.hpp"
#include "mwrender/rendermanager.hpp"
#include "mwworld/worldimp.hpp"
#include "mwscript/interpretercontext.hpp"
#include "mwmechanics/mechanicsmanagerimp.hpp"
#include "mwdialogue/dialoguemanagerimp.hpp"
#include "mwdialogue/journalimp.hpp"
#include "mwclass/classes.hpp"
#include "mwstate/menu.hpp"
#include "mwstate/loading.hpp"

#include <components/files/configurationmanager.hpp>
#include <components/fallback/fallback.hpp>
#include <components/compiler/extensions0.hpp>

namespace OMW
{
class Engine;

/*
 * Loads content files.
 */
class Boot : public MWState::Loading
{
    Engine &mEngine;
    MWBase::Environment &mEnvironment;
    bool mPlayedLogo = false;
public:
    Boot(Engine &engine)
        : mEngine(engine), mEnvironment(mEngine.mEnvironment) {}
    bool run(float dt) override
    {
        MWClass::registerClasses();

        //playLogoWhileLoadingWorld
        bool threadRunning = Loading::run(dt);
        if (!mPlayedLogo && !mEngine.mSkipMenu)
        {
            const std::string& logo = Fallback::Map::getString("Movies_Company_Logo");
            if (!logo.empty())
                mEnvironment.getWindowManager()->playVideo(logo, true);
            mPlayedLogo = true;
            return true;
        }
        if (threadRunning)
            return true;
        else if (abort)
            return false;

        //initializeWorldDependentObjects
        mEngine.mTranslationDataStorage.setEncoder(mEngine.mEncoder.get());
        for (size_t i = 0; i < mEngine.mContentFiles.size(); i++)
            mEngine.mTranslationDataStorage.loadTranslationData(mEngine.mFileCollections, mEngine.mContentFiles[i]);

        Compiler::registerExtensions (mEngine.mExtensions);
        mEngine.mScriptContext.reset(new MWScript::CompilerContext (MWScript::CompilerContext::Type_Full));
        mEngine.mScriptContext->setExtensions (&mEngine.mExtensions);

        mEngine.mScriptManager = std::make_unique<MWScript::ScriptManager>(mEnvironment.getWorld()->getStore(), *mEngine.mScriptContext, mEngine.mWarningsMode,
            mEngine.mScriptBlacklistUse ? mEngine.mScriptBlacklist : std::vector<std::string>());
        mEnvironment.setScriptManager(*mEngine.mScriptManager);

        mEngine.mMechanicsManager = std::make_unique<MWMechanics::MechanicsManager>();
        mEnvironment.setMechanicsManager(*mEngine.mMechanicsManager);

        mEngine.mJournal = std::make_unique<MWDialogue::Journal>();
        mEnvironment.setJournal(*mEngine.mJournal);

        mEngine.mDialogueManager = std::make_unique<MWDialogue::DialogueManager>(mEngine.mExtensions, mEngine.mTranslationDataStorage);
        mEnvironment.setDialogueManager(*mEngine.mDialogueManager);

        auto &wm = mEngine.mWindowManager;
        wm->setStore(mEnvironment.getWorld()->getStore());
        wm->initUI(mEngine.mRenderManager->createMapScene());
        if (mEngine.mCompileAll)
        {
            std::pair<int, int> result = mEnvironment.getScriptManager()->compileAll();
            if (result.first)
                Log(Debug::Info)
                    << "compiled " << result.second << " of " << result.first << " scripts ("
                    << 100*static_cast<double> (result.second)/result.first
                    << "%)";
        }
        if (mEngine.mCompileAllDialogue)
        {
            std::pair<int, int> result = MWDialogue::ScriptTest::compileAll(&mEngine.mExtensions, mEngine.mWarningsMode);
            if (result.first)
                Log(Debug::Info)
                    << "compiled " << result.second << " of " << result.first << " dialogue script/actor combinations a("
                    << 100*static_cast<double> (result.second)/result.first
                    << "%)";
        }

        mEngine.mLuaManager->init();
        mEngine.mLuaManager->loadPermanentStorage(mEngine.mCfgMgr.getUserConfigPath().string());

        mEnvironment.getStateManager()->pushGameState(std::make_shared<Game>(mEngine));

        // Start the game
        if (!mEngine.mSaveGameFile.empty())
            mEnvironment.getStateManager()->loadGame(mEngine.mSaveGameFile);
        else if (!mEngine.mSkipMenu)
        {
            // start in main menu
            mEnvironment.getStateManager()->pushGameState(std::make_shared<MWState::Menu>());
            mEnvironment.getSoundManager()->playTitleMusic();
            const std::string& logo = Fallback::Map::getString("Movies_Morrowind_Logo");
            if (!logo.empty())
                mEnvironment.getWindowManager()->playVideo(logo, false);
        }
        else
            mEnvironment.getStateManager()->newGame (!mEngine.mNewGame);

        if (!mEngine.mStartupScript.empty() && mEnvironment.getStateManager()->getState() == MWBase::StateManager::State_Running)
            mEnvironment.getWindowManager()->executeInConsole(mEngine.mStartupScript);
        return false;
    }
    void threadLoad() override
    {
        mEngine.mWorld = std::make_unique<MWWorld::World>(*mEngine.mRenderManager, mEngine.mResourceSystem.get(), mEngine.mActivationDistanceOverride, mEngine.mCellName, mEngine.mStartupScript, mEngine.mCfgMgr.getUserDataPath().string());
        mEnvironment.setWorld(*mEngine.mWorld);
        mEngine.mWorld->loadContent(mEngine.mFileCollections, mEngine.mContentFiles, mEngine.mGroundcoverFiles, mEngine.mEncoder.get(), *this);
    }
};
}
