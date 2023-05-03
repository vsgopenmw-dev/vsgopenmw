#ifndef GAME_STATE_STATEMANAGER_H
#define GAME_STATE_STATEMANAGER_H

#include <filesystem>
#include <map>

#include "../mwbase/statemanager.hpp"

#include "charactermanager.hpp"

namespace MWRender
{
    class ScreenshotInterface;
}
namespace MWState
{
    /*
     * Encapsulates game states.
     */
    class StateManager : public MWBase::StateManager
    {
        MWRender::ScreenshotInterface& mScreenshotInterface;
        bool mQuitRequest;
        State mState;
        CharacterManager mCharacterManager;
        double mTimePlayed;

        std::vector<std::shared_ptr<GameState>> mGameStates;

    public:
        StateManager(const std::filesystem::path& saves, const std::vector<std::string>& contentFiles,
            MWRender::ScreenshotInterface& screenshotInterface);

        void requestQuit() override;

        bool hasQuitRequest() const override;

        void askLoadRecent() override;

        State getState() const override;

        std::shared_ptr<GameState> getGameState();
        void pushGameState(std::shared_ptr<GameState> gameState) override;
        void popGameState(std::shared_ptr<GameState> gameState) override;

        void cleanup(bool force = false);

        void cleanupAndPush(std::shared_ptr<GameState> next, bool forceCleanup = false);

        void newGame(bool /*vsgopenmw-delete-me*/bypass = false) override;
        void bypassNewGame(const std::string& startCell);

        ///< Start a new game.
        ///
        /// \param bypass Skip new game mechanics.

        void endGame() override;

        void resumeGame() override;

        void deleteGame(const MWState::Character* character, const MWState::Slot* slot) override;
        ///< Delete a saved game slot from this character. If all save slots are deleted, the character will be deleted
        ///< too.

        void saveGame(const std::string& description, const Slot* slot = nullptr) override;
        ///< Write a saved game to \a slot or create a new slot if \a slot == 0.
        ///
        /// \note Slot must belong to the current character.

        /// Saves a file, using supplied filename, overwritting if needed
        /** This is mostly used for quicksaving and autosaving, for they use the same name over and over again
            \param name Name of save, defaults to "Quicksave"**/
        void quickSave(std::string name = "Quicksave") override;

        /// Loads the last saved file
        /** Used for quickload **/
        void quickLoad() override;

        void loadGame(const std::filesystem::path& filepath) override;
        ///< Load a saved game directly from the given file path. This will search the CharacterManager
        /// for a Character containing this save file, and set this Character current if one was found.
        /// Otherwise, a new Character will be created.

        void loadGame(const Character* character, const std::filesystem::path& filepath) override;
        ///< Load a saved game file belonging to the given character.

        void error(const std::string& what);

        void mainMenu();

        Character* getCurrentCharacter() override;
        ///< @note May return null.

        CharacterIterator characterBegin() override;
        ///< Any call to SaveGame and getCurrentCharacter can invalidate the returned
        /// iterator.

        CharacterIterator characterEnd() override;

        void update(float duration);
    };
}

#endif
