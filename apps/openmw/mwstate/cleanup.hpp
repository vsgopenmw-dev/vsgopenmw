// vsgopenmw-unity-build

#include "operation.hpp"
#include "statemanagerimp.hpp"

namespace MWState
{
    struct Cleanup : public Operation
    {
        MWState::StateManager& mStateManager;
        std::shared_ptr<GameState> next;
        bool force{};
        Cleanup(MWState::StateManager& stateMgr)
            : mStateManager(stateMgr)
        {
        }
        bool run(float dt) override
        {
            mStateManager.cleanup(force);
            if (next)
                mStateManager.pushGameState(next);
            return false;
        }
    };
}
