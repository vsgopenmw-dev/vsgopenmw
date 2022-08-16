//vsgopenmw-unity-build

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "statemanagerimp.hpp"
#include "choice.hpp"
#include "menu.hpp"

namespace MWState
{
class AskLoadRecent : public Choice
{
    StateManager &mStateManager;
    const Character *mCharacter;
public:
    AskLoadRecent(StateManager &manager, const MWState::Character* character) : mStateManager(manager), mCharacter(character)
    {
        buttons = {"#{sYes}", "#{sNo}"};
        std::string tag("%s");
        message = MWBase::Environment::get().getWindowManager()->getGameSettingString("sLoadLastSaveMsg", tag);
        MWState::Slot lastSave = *mCharacter->begin();
        size_t pos = message.find(tag);
        if (pos != std::string::npos)
            message.replace(pos, tag.length(), lastSave.mProfile.mDescription);
    }
    bool run(float dt) override
    {
        if (mStateManager.hasQuitRequest()) return false;
        if (!Choice::run(dt))
        {
            if(pressedButton == 0)
            {
                MWState::Slot lastSave = *mCharacter->begin();
                mStateManager.loadGame(mCharacter, lastSave.mPath.string());
            }
            else
                mStateManager.pushGameState(std::make_shared<Menu>());
            return false;
        }
        return true;
    }
};
}
