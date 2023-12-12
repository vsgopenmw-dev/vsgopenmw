#include "statemanagerimp.hpp"

#include <filesystem>

#include <components/esm3/cellid.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadclas.hpp>

#include <components/files/conversion.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwworld/globals.hpp"

#include "askloadrecent.hpp"
#include "cleanup.hpp"
#include "loadgame.hpp"
#include "loadingscreen.hpp"
#include "newgame.hpp"
#include "quicksavemanager.hpp"
#include "savegame.hpp"

namespace MWState
{

    void MWState::StateManager::cleanup(bool force)
    {
        if (mState != State_NoGame || force)
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

    MWState::StateManager::StateManager(const std::filesystem::path& saves,
        const std::vector<std::string>& contentFiles, MWRender::ScreenshotInterface& screenshotInterface)
        : mScreenshotInterface(screenshotInterface)
        , mQuitRequest(false)
        , mState(State_NoGame)
        , mCharacterManager(saves, contentFiles)
        , mTimePlayed(0)
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
        endGame(); //;
        const MWState::Character* character = getCurrentCharacter();
        if (!character || character->begin() == character->end())
            pushGameState(std::make_shared<Menu>());
        else
            pushGameState(std::make_shared<AskLoadRecent>(*this, character));
    }

    MWState::StateManager::State MWState::StateManager::getState() const
    {
        return mState;
    }

    void StateManager::cleanupAndPush(std::shared_ptr<GameState> next, bool forceCleanup)
    {
        auto cleanup = std::make_shared<Cleanup>(*this);
        cleanup->next = next;
        cleanup->force = forceCleanup;
        pushGameState(cleanup);
    }

    void MWState::StateManager::newGame(bool bypass)
    {
        cleanupAndPush(std::make_shared<NewGame>(*this, mState, std::nullopt));
    }

    void MWState::StateManager::bypassNewGame(const std::string& startCell)
    {
        cleanupAndPush(std::make_shared<NewGame>(*this, mState, startCell));
    }

    void MWState::StateManager::endGame()
    {
        mState = State_Ended;
    }

    void MWState::StateManager::resumeGame()
    {
        mState = State_Running;
    }

    void MWState::StateManager::saveGame(std::string_view description, const Slot* slot)
    {
        if (mState != State_Running)
            return;
        MWBase::Environment::get().getWindowManager()->asyncPrepareSaveMap();
        MWBase::Environment::get().getLuaManager()->applyDelayedActions();

        MWState::Character* character = getCurrentCharacter();
        if (!character)
        {
            MWWorld::ConstPtr player = MWMechanics::getPlayer();
            std::string name = player.get<ESM::NPC>()->mBase->mName;
            character = mCharacterManager.createCharacter(name);
            mCharacterManager.setCurrentCharacter(character);
        }

        auto gameState = std::make_shared<SaveGame>();
        gameState->screenshotInterface = &mScreenshotInterface;
        gameState->timePlayed = mTimePlayed;
        gameState->character = character;
        gameState->slot = slot;
        gameState->description = description;

        auto loadingScreen = MWBase::Environment::get().getWindowManager()->createLoadingScreen();
        loadingScreen->loading = gameState;
        loadingScreen->setWallpaper(false);
        pushGameState(loadingScreen);
    }

    void MWState::StateManager::quickSave(std::string name)
    {
        if (!(mState == State_Running
                && MWBase::Environment::get().getWorld()->getGlobalInt(MWWorld::Globals::sCharGenState) == -1 // char gen
                && MWBase::Environment::get().getWindowManager()->isSavingAllowed()))
        {
            // You can not save your game right now
            MWBase::Environment::get().getWindowManager()->messageBox("#{OMWEngine:SaveGameDenied}");
            return;
        }

        int maxSaves = Settings::Manager::getInt("max quicksaves", "Saves");
        if (maxSaves < 1)
            maxSaves = 1;

        Character* currentCharacter = getCurrentCharacter(); // Get current character
        QuickSaveManager saveFinder = QuickSaveManager(name, maxSaves);

        if (currentCharacter)
        {
            for (auto& save : *currentCharacter)
            {
                // Visiting slots allows the quicksave finder to find the oldest quicksave
                saveFinder.visitSave(&save);
            }
        }

        // Once all the saves have been visited, the save finder can tell us which
        // one to replace (or create)
        saveGame(name, saveFinder.getNextQuickSaveSlot());
    }

    void MWState::StateManager::loadGame(const std::filesystem::path& filepath)
    {
        for (const auto& character : mCharacterManager)
        {
            for (const auto& slot : character)
            {
                if (slot.mPath == filepath)
                {
                    loadGame(&character, slot.mPath);
                    return;
                }
            }
        }

        MWState::Character* character = getCurrentCharacter();
        loadGame(character, filepath);
    }

    void MWState::StateManager::loadGame(const Character* character, const std::filesystem::path& filepath)
    {
        auto gameState = std::make_shared<LoadGame>(mTimePlayed, mCharacterManager, mState, *this);
        gameState->character = character;
        gameState->filepath = filepath;

        auto loadingScreen = MWBase::Environment::get().getWindowManager()->createLoadingScreen();
        loadingScreen->loading = gameState;
        cleanupAndPush(loadingScreen);
    }

    void MWState::StateManager::mainMenu()
    {
        cleanupAndPush(std::make_shared<Menu>());
    }

    void MWState::StateManager::error(const std::string& what)
    {
        cleanupAndPush(std::make_shared<Error>(what), true);
    }

    void MWState::StateManager::quickLoad()
    {
        if (Character* currentCharacter = getCurrentCharacter())
        {
            if (currentCharacter->begin() == currentCharacter->end())
                return;
            loadGame(currentCharacter, currentCharacter->begin()->mPath); // Get newest save
        }
    }

    void MWState::StateManager::deleteGame(const MWState::Character* character, const MWState::Slot* slot)
    {
        mCharacterManager.deleteSlot(character, slot);
    }

    MWState::Character* MWState::StateManager::getCurrentCharacter()
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

    void MWState::StateManager::update(float duration)
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
