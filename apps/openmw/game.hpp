//vsgopenmw-unity-build

#include <memory>

#include "mwscript/interpretercontext.hpp"
#include "mwworld/localscripts.hpp"
#include "mwworld/class.hpp"
#include "mwmechanics/creaturestats.hpp"

#include "mwgui/windowmanagerimp.hpp"
#include "mwlua/luamanagerimp.hpp"
#include "mwworld/worldimp.hpp"
#include "mwmechanics/mechanicsmanagerimp.hpp"
#include "mwstate/statemanagerimp.hpp"
#include "mwstate/gamestate.hpp"
#include "mwscript/scriptmanagerimp.hpp"

#include "luaworker.hpp"
#include "vsgengine.hpp"

#include <osg/Stats>
#include <osg/ref_ptr>

namespace OMW
{
class LuaWorker;
class Engine;

/*
 * Updates running game.
 */
class Game : public MWState::GameState
{
    Engine &mEngine;
    std::unique_ptr<LuaWorker> mLuaWorker;
    void executeLocalScripts()
    {
        auto& localScripts = mEngine.mWorld->getLocalScripts();
        localScripts.startIteration();
        std::pair<std::string, MWWorld::Ptr> script;
        while (localScripts.getNext(script))
        {
            MWScript::InterpreterContext interpreterContext (
                &script.second.getRefData().getLocals(), script.second);
            mEngine.mScriptManager->run (script.first, interpreterContext);
        }
    }
public:
    Game(Engine &e) : mEngine(e)
    {
        mLuaWorker = std::make_unique<LuaWorker>(mEngine.mLuaManager.get());
    }
    bool run(float dt) override
    {
        if (mEngine.mStateManager->hasQuitRequest())
            return false;
        mLuaWorker->finishUpdate();
        // Should be called after input manager update and before any change to the game world.
        // It applies to the game world queued changes from the previous frame.
        mEngine.mLuaManager->synchronizedUpdate();

        if (mEngine.mWorld->getScriptsEnabled())
        {
            executeLocalScripts();
            mEngine.mScriptManager->getGlobalScripts().run();
        }

        mEngine.mWorld->markCellAsUnchanged();
        bool guiActive = mEngine.mWindowManager->isGuiMode();
        if (!guiActive)
        {
            double hours = (dt * mEngine.mWorld->getTimeScaleFactor()) / 3600.0;
            mEngine.mWorld->advanceTime(hours, true);
            mEngine.mWorld->rechargeItems(dt, true);
        }

        mEngine.mMechanicsManager->update(dt, guiActive);

        mEngine.mStateManager->/*advanceTimePlayed*/update(dt);
        auto player = mEngine.mWorld->getPlayerPtr();
        if(!guiActive && player.getClass().getCreatureStats(player).isDead())
            mEngine.mStateManager->endGame();

        osg::ref_ptr<osg::Stats> dummyStats = new osg::Stats ("dummy");

        mEngine.mWorld->updatePhysics(dt, guiActive, osg::Timer_t(), 0, *dummyStats);

        mEngine.mWorld->update(dt, guiActive);

        mEngine.mWindowManager->update(dt);
        mEngine.mWorld->updateWindowManager();
        mLuaWorker->allowUpdate();
        return true;
    }
};
}
