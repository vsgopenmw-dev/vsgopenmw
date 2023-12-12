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
    class ChangeToStartingCell : public Operation
    {
    public:
        int numFrames = 5;
        bool run(float dt) override
        {
            auto world = MWBase::Environment::get().getWorld();
            if (!world->getPlayerPtr().isInCell())
            {
                if (numFrames-- > 0)
                {
                    MWBase::Environment::get().getScriptManager()->getGlobalScripts().run(dt);
                    return true;
                }
                else
                {
                    ESM::ExteriorCellLocation loc = { 0, 0, ESM::Cell::sDefaultWorldspaceId };
                    auto& cell = MWBase::Environment::get().getWorldModel()->getExterior(loc);
                    auto p = ESM::indexToPosition(loc, true);
                    ESM::Position pos;
                    pos.pos[0] = p.x();
                    pos.pos[1] = p.y();
                    pos.pos[2] = 0.f; // should be adjusted automatically (adjustPlayerPos=true)
                    pos.rot[0] = 0.f;
                    pos.rot[1] = 0.f;
                    pos.rot[2] = 0.f;
                    MWBase::Environment::get().getWorld()->changeToCell(cell.getCell()->getId(), pos, true);
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
                    mStateManager.pushGameState(std::make_shared<ChangeToStartingCell>());

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
                        auto c = MWBase::Environment::get().getWorld()->findExteriorPosition(startCell, pos);
                        if (!c.empty())
                        {
                            world->changeToCell(c, pos, true);
                        }
                        else
                        {
                            world->findInteriorPosition(startCell, pos);
                            world->changeToInteriorCell(startCell, pos, true);
                        }
                    }
                    else
                        mStateManager.pushGameState(std::make_shared<ChangeToStartingCell>());
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
