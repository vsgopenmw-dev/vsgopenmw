// vsgopenmw-unity-build

#include <memory>

#include <components/compiler/extensions.hpp>
#include <components/compiler/extensions0.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/vsgutil/updatethreads.hpp>

#include "mwdialogue/dialoguemanagerimp.hpp"
#include "mwdialogue/journalimp.hpp"
#include "mwdialogue/scripttest.hpp"
#include "mwgui/windowmanagerimp.hpp"
#include "mwlua/luamanagerimp.hpp"
#include "mwmechanics/creaturestats.hpp"
#include "mwmechanics/mechanicsmanagerimp.hpp"
#include "mwscript/interpretercontext.hpp"
#include "mwscript/scriptmanagerimp.hpp"
#include "mwstate/gamestate.hpp"
#include "mwstate/statemanagerimp.hpp"
#include "mwworld/class.hpp"
#include "mwworld/datetimemanager.hpp"
#include "mwworld/localscripts.hpp"
#include "mwworld/worldimp.hpp"
#include "mwworld/scene.hpp"
#include "mwclass/classes.hpp"
#include "mwrender/rendermanager.hpp"

#include "vsgengine.hpp"

namespace OMW
{
    /*
     * Updates running game.
     */
    class Game : public MWState::GameState
    {
        void executeLocalScripts(float dt)
        {
            auto& localScripts = mWorld->getLocalScripts();
            localScripts.startIteration();
            std::pair<ESM::RefId, MWWorld::Ptr> script;
            while (localScripts.getNext(script))
            {
                MWScript::InterpreterContext interpreterContext(&script.second.getRefData().getLocals(), script.second);
                interpreterContext.dt = dt;
                mScriptManager->run(script.first, interpreterContext);
            }
        }

        Engine& mEngine;
        MWBase::Environment& mEnvironment;
        Compiler::Extensions mExtensions;
        std::unique_ptr<Compiler::Context> mScriptContext;
        std::unique_ptr<MWWorld::World> mWorld;
        std::unique_ptr<MWScript::ScriptManager> mScriptManager;
        std::unique_ptr<MWDialogue::Journal> mJournal;
        std::unique_ptr<MWDialogue::DialogueManager> mDialogueManager;
        std::unique_ptr<MWMechanics::MechanicsManager> mMechanicsManager;
        std::unique_ptr<MWLua::LuaManager> mLuaManager;

    public:
        Game(Engine& e, std::unique_ptr<MWWorld::World>& world, const Arguments& args)
            : mEngine(e)
            , mEnvironment(e.mEnvironment)
            , mWorld(std::move(world))
        {
            mLuaManager = std::make_unique<MWLua::LuaManager>(mEngine.mVFS.get(), args.resourceDir / "lua_libs");
            mEnvironment.setLuaManager(*mLuaManager);

            Compiler::registerExtensions(mExtensions);
            mScriptContext.reset(new MWScript::CompilerContext(MWScript::CompilerContext::Type_Full));
            mScriptContext->setExtensions(&mExtensions);
            mScriptManager = std::make_unique<MWScript::ScriptManager>(
                mEnvironment.getWorld()->getStore(), *mScriptContext, args.warningsMode, args.scriptBlacklist);
            mEnvironment.setScriptManager(*mScriptManager);

            mMechanicsManager = std::make_unique<MWMechanics::MechanicsManager>();
            mMechanicsManager->setUpdateThreads(mEngine.mRenderManager->getUpdateThreads());
            mEnvironment.setMechanicsManager(*mMechanicsManager);

            mJournal = std::make_unique<MWDialogue::Journal>();
            mEnvironment.setJournal(*mJournal);

            mDialogueManager
                = std::make_unique<MWDialogue::DialogueManager>(mExtensions, mEngine.mTranslationDataStorage);
            mEnvironment.setDialogueManager(*mDialogueManager);

            auto& wm = mEngine.mWindowManager;
            wm->setStore(mEnvironment.getWorld()->getStore());
            wm->initUI(mEngine.mRenderManager->getMap(), mEngine.mRenderManager->getWorldMap(), mEngine.mRenderManager->getPreview());

            mLuaManager->init(mEngine.mRenderManager.get());
            mLuaManager->loadPermanentStorage(mEngine.mCfgMgr.getUserConfigPath());

            mWorld->setupPlayer();

            if (args.compileAll)
            {
                std::pair<int, int> result = mEnvironment.getScriptManager()->compileAll();
                if (result.first)
                    std::cout << "compiled " << result.second << " of " << result.first << " scripts ("
                                     << 100 * static_cast<double>(result.second) / result.first << "%)" << std::endl;
            }
            if (args.compileAllDialogue)
            {
                std::pair<int, int> result = MWDialogue::ScriptTest::compileAll(&mExtensions, args.warningsMode);
                if (result.first)
                    std::cout << "compiled " << result.second << " of " << result.first
                                     << " dialogue script/actor combinations ("
                                     << 100 * static_cast<double>(result.second) / result.first << "%)" << std::endl;
            }

        }
        ~Game()
        {
            if (mLuaManager)
                mLuaManager->savePermanentStorage(mEngine.mCfgMgr.getUserConfigPath().string());
        }
        bool run(float dt) override
        {
            if (mEngine.mStateManager->hasQuitRequest())
                return false;

            try
            {
                dt = std::min(dt, 0.2f);
                mLuaManager->update(dt);
                mLuaManager->synchronizedUpdate(dt);

                if (mWorld->getScriptsEnabled())
                {
                    executeLocalScripts(dt);
                    mScriptManager->getGlobalScripts().run(dt);
                }

                mWorld->getWorldScene().markCellAsUnchanged();
                bool guiActive = mEngine.mWindowManager->isGuiMode();
                showCursor = guiActive;
                if (!guiActive)
                {
                    double hours = (dt * mWorld->getTimeManager()->getGameTimeScale()) / 3600.0;
                    mWorld->advanceTime(hours, true);
                    mWorld->rechargeItems(dt, true);
                }

                mMechanicsManager->update(dt, guiActive);

                mEngine.mStateManager->/*advanceTimePlayed*/ update(dt);

                mWorld->updatePhysics(dt, guiActive);
                mWorld->update(dt, guiActive);

                mEngine.mWindowManager->update(dt);
                mWorld->updateWindowManager();

                mEngine.mSoundManager->updateRegionSound(dt);
                mEngine.mSoundManager->updateWaterSound();
            }
            catch (const std::exception& e)
            {
                std::cerr << "!Game::run(" << e.what() << ")" << std::endl;
                mEngine.mStateManager->saveGame("Emergency Save");
                return false;
            }
            return true;
        }
    };
}
