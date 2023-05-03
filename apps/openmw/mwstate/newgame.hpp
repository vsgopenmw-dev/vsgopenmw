// vsgopenmw-unity-build

#include <sstream>
#include <optional>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwscript/globalscripts.hpp"

#include <components/debug/debuglog.hpp>
#include <components/fallback/fallback.hpp>

#include "error.hpp"
#include "operation.hpp"
#include "statemanagerimp.hpp"

namespace MWState
{
    class WaitForStartingCell : public Operation
    {
    public:
        int numRetry = 0;
        WaitForStartingCell(int in_numRetry = 0)
            : numRetry(in_numRetry)
        {
        }
        bool run(float dt) override
        {
            auto world = MWBase::Environment::get().getWorld();
            if (!world->getPlayerPtr().isInCell())
            {
                if (numRetry < 5)
                {
                    MWBase::Environment::get().getScriptManager()->getGlobalScripts().run(dt);

                    //requiresRender = false; return true;
                    MWBase::Environment::get().getStateManager()->pushGameState(
                        std::make_shared<WaitForStartingCell>(++numRetry));
                }
                else
                {
                    ESM::Position pos{};
                    world->indexToPosition(0, 0, pos.pos[0], pos.pos[1], true);
                    world->changeToExteriorCell(pos, true);
                }
            }
            return false;
        }
    };

    class NewGame : public Operation
    {
        StateManager& mStateManager;
        StateManager::State& mState;

    public:
        NewGame(StateManager& stateMgr, StateManager::State& out_state, std::optional<std::string> in_bypassStartCell)
            : mStateManager(stateMgr)
            , mState(out_state)
            , bypassStartCell(in_bypassStartCell)
        {
        }
        std::optional<std::string> bypassStartCell;
        bool run(float dt)
        {
            Log(Debug::Info) << "Starting a new game";
            try
            {
                auto world = MWBase::Environment::get().getWorld();
                MWBase::Environment::get().getScriptManager()->getGlobalScripts().addStartup();
                MWBase::Environment::get().getLuaManager()->newGameStarted();
                world->startNewGame(bypassStartCell.has_value());
                MWBase::Environment::get().getWindowManager()->fadeScreenIn(1);

                if (!bypassStartCell.has_value())
                {
                    mStateManager.pushGameState(std::make_shared<WaitForStartingCell>());

                    std::string_view video = Fallback::Map::getString("Movies_New_Game");
                    if (!video.empty())
                        MWBase::Environment::get().getWindowManager()->playVideo(video, true);
                }
                else
                {
                    auto startCell = *bypassStartCell;
                    if (!startCell.empty())
                    {
                        ESM::Position pos;
                        if (MWBase::Environment::get().getWorld()->findExteriorPosition(startCell, pos))
                        {
                            world->changeToExteriorCell(pos, true);
                            world->adjustPosition(world->getPlayerPtr(), false);
                        }
                        else
                        {
                            world->findInteriorPosition(startCell, pos);
                            world->changeToInteriorCell(startCell, pos, true);
                        }
                    }
                    else
                        mStateManager.pushGameState(std::make_shared<WaitForStartingCell>());
                }
                mState = StateManager::State_Running;
            }
            catch (std::exception& e)
            {
                mStateManager.error(std::string("Failed to start new game: ") + e.what());
            }
            return false;
        }
    };
}
