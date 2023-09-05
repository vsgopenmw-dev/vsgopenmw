// vsgopenmw-unity-build

#include "operation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWState
{
    struct ExecuteScript : public Operation
    {
        ExecuteScript(const std::string& f) : filename(f) {}
        std::string filename;
        bool run(float dt)
        {
            if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_Running)
                MWBase::Environment::get().getWindowManager()->executeInConsole(filename);
            return false;
        }
    };
}
