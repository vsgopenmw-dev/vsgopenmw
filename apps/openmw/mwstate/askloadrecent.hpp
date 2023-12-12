// vsgopenmw-unity-build

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include <components/l10n/manager.hpp>

#include "choice.hpp"
#include "statemanagerimp.hpp"

namespace MWState
{
    class AskLoadRecent : public Choice
    {
        StateManager& mStateManager;
        const Character* mCharacter;

    public:
        AskLoadRecent(StateManager& manager, const MWState::Character* character)
            : mStateManager(manager)
            , mCharacter(character)
        {
            buttons = { "#{Interface:Yes}", "#{Interface:No}" };
            std::string_view tag("%s");
            std::string message
                = MWBase::Environment::get().getL10nManager()->getMessage("OMWEngine", "AskLoadLastSave");

            MWState::Slot lastSave = *mCharacter->begin();
            size_t pos = message.find(tag);
            if (pos != std::string::npos)
                message.replace(pos, tag.length(), lastSave.mProfile.mDescription);
        }
        bool run(float dt) override
        {
            if (mStateManager.hasQuitRequest())
                return false;
            if (!Choice::run(dt))
            {
                if (pressedButton == 0)
                {
                    MWState::Slot lastSave = *mCharacter->begin();
                    mStateManager.loadGame(mCharacter, lastSave.mPath.string());
                }
                else
                    mStateManager.mainMenu();
                return false;
            }
            return true;
        }
    };
}
