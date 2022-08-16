#include "statemanagerimp.hpp"

#include <filesystem>

#include <components/debug/debuglog.hpp>

#include <components/esm3/cellid.hpp>
#include <components/esm3/loadcell.hpp>

#include <components/settings/settings.hpp>

#include <boost/filesystem/path.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/luamanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwscript/globalscripts.hpp"

#include "loadingscreen.hpp"
#include "loadgame.hpp"
#include "savegame.hpp"
#include "askloadrecent.hpp"
#include "quicksavemanager.hpp"

namespace MWState
{

void MWState::StateManager::cleanup (bool force)
{
    if (mState!=State_NoGame || force)
    {
        MWBase::Environment::get().getSoundManager()->clear();
        MWBase::Environment::get().getDialogueManager()->clear();
        MWBase::Environment::get().getJournal()->clear();
        MWBase::Environment::get().getScriptManager()->clear();
        MWBase::Environment::get().getWindowManager()->clear();
        MWBase::Environment::get().getWorld()->clear();
        MWBase::Environment::get().getInputManager()->clear();
        MWBase::Environment::get().getMechanicsManager()->clear();

        mState = State_NoGame;
        mCharacterManager.setCurrentCharacter(nullptr);
        mTimePlayed = 0;

        MWMechanics::CreatureStats::cleanup();
    }
    MWBase::Environment::get().getLuaManager()->clear();
}

MWState::StateManager::StateManager (const boost::filesystem::path& saves, const std::vector<std::string>& contentFiles)
: mQuitRequest (false), mState (State_NoGame), mCharacterManager (saves, contentFiles), mTimePlayed (0)
{

}

void MWState::StateManager::requestQuit()
{
    mQuitRequest = true;
}

bool MWState::StateManager::hasQuitRequest() const
{
    return mQuitRequest;
}

void MWState::StateManager::askLoadRecent()
{
    if (MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_MainMenu)
        return;

    const MWState::Character* character = getCurrentCharacter();
    if(!character || character->begin() == character->end())
        pushGameState(std::make_shared<Menu>());
    else
        pushGameState(std::make_shared<AskLoadRecent>(*this, character));
}

MWState::StateManager::State MWState::StateManager::getState() const
{
    return mState;
}

void MWState::StateManager::newGame (bool bypass)
{
    cleanup();

    if (!bypass)
        MWBase::Environment::get().getWindowManager()->setNewGame (true);

    try
    {
        Log(Debug::Info) << "Starting a new game";
        MWBase::Environment::get().getScriptManager()->getGlobalScripts().addStartup();
        MWBase::Environment::get().getLuaManager()->newGameStarted();
        MWBase::Environment::get().getWorld()->startNewGame (bypass);

        mState = State_Running;

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0);
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(1);

        if (!bypass)
            pushGameState(makeLoadingScreen<Loading>()); //mEngine.compile();
    }
    catch (std::exception& e)
    {
        std::stringstream error;
        error << "Failed to start new game: " << e.what();

        cleanup (true);
        pushGameState(std::make_shared<Error>(error.str()));
    }
}

void MWState::StateManager::endGame()
{
    mState = State_Ended;
}

void MWState::StateManager::resumeGame()
{
    mState = State_Running;
}

void MWState::StateManager::saveGame (const std::string& description, const Slot *slot)
{
    MWBase::Environment::get().getWindowManager()->asyncPrepareSaveMap();

    MWState::Character* character = getCurrentCharacter();
    if (!character)
    {
        MWWorld::ConstPtr player = MWMechanics::getPlayer();
        std::string name = player.get<ESM::NPC>()->mBase->mName;
        character = mCharacterManager.createCharacter(name);
        mCharacterManager.setCurrentCharacter(character);
    }

    auto gameState = std::make_shared<SaveGame>();
    gameState->timePlayed = mTimePlayed;
    gameState->character = character;
    gameState->slot = slot;
    gameState->description = description;

    auto loadingScreen = std::make_shared<MWState::LoadingScreen>(gameState);
    loadingScreen->setWallpaper(false);
    loadingScreen->requiresCompile = false;
    pushGameState(loadingScreen);
}

void MWState::StateManager::quickSave (std::string name)
{
    if (!(mState==State_Running &&
        MWBase::Environment::get().getWorld()->getGlobalInt ("chargenstate")==-1 // char gen
            && MWBase::Environment::get().getWindowManager()->isSavingAllowed()))
    {
        //You can not save your game right now
        MWBase::Environment::get().getWindowManager()->messageBox("#{sSaveGameDenied}");
        return;
    }

    int maxSaves = Settings::Manager::getInt("max quicksaves", "Saves");
    if(maxSaves < 1)
        maxSaves = 1;

    Character* currentCharacter = getCurrentCharacter(); //Get current character
    QuickSaveManager saveFinder = QuickSaveManager(name, maxSaves);

    if (currentCharacter)
    {
        for (auto& save : *currentCharacter)
        {
            //Visiting slots allows the quicksave finder to find the oldest quicksave
            saveFinder.visitSave(&save);
        }
    }

    //Once all the saves have been visited, the save finder can tell us which
    //one to replace (or create)
    saveGame(name, saveFinder.getNextQuickSaveSlot());
}

void MWState::StateManager::loadGame(const std::string& filepath)
{
    for (const auto& character : mCharacterManager)
    {
        for (const auto& slot : character)
        {
            if (slot.mPath == boost::filesystem::path(filepath))
            {
                loadGame(&character, slot.mPath.string());
                return;
            }
        }
    }

    MWState::Character* character = getCurrentCharacter();
    loadGame(character, filepath);
}

void MWState::StateManager::loadGame (const Character *character, const std::string& filepath)
{
    cleanup();
    auto gameState = std::make_shared<LoadGame>(mTimePlayed, mCharacterManager, mState, *this);
    gameState->character = character;
    gameState->filepath = filepath;
    pushGameState(std::make_shared<LoadingScreen>(gameState));
}

void MWState::StateManager::quickLoad()
{
    if (Character* currentCharacter = getCurrentCharacter ())
    {
        if (currentCharacter->begin() == currentCharacter->end())
            return;
        loadGame (currentCharacter, currentCharacter->begin()->mPath.string()); //Get newest save
    }
}

void MWState::StateManager::deleteGame(const MWState::Character *character, const MWState::Slot *slot)
{
    mCharacterManager.deleteSlot(character, slot);
}

MWState::Character *MWState::StateManager::getCurrentCharacter ()
{
    return mCharacterManager.getCurrentCharacter();
}

MWState::StateManager::CharacterIterator MWState::StateManager::characterBegin()
{
    return mCharacterManager.begin();
}

MWState::StateManager::CharacterIterator MWState::StateManager::characterEnd()
{
    return mCharacterManager.end();
}

void MWState::StateManager::update (float duration)
{
    mTimePlayed += duration;
}

std::shared_ptr<GameState> StateManager::getGameState()
{
    if (mGameStates.empty())
        return {};
    return mGameStates.back();
}

void StateManager::popGameState(std::shared_ptr<GameState> state)
{
    auto i = std::find(mGameStates.begin(), mGameStates.end(), state);
    if (i != mGameStates.end())
        mGameStates.erase(i);
}

void StateManager::pushGameState(std::shared_ptr<GameState> state)
{
    mGameStates.push_back(state);
}

}
