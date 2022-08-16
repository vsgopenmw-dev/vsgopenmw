//vsgopenmw-unity-build

#include <filesystem>

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
#include "../mwmechanics/actorutil.hpp"
#include "../mwscript/globalscripts.hpp"
#include "../mwworld/cellstore.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/settings/settings.hpp>

#include "statemanagerimp.hpp"
#include "loading.hpp"
#include "choice.hpp"
#include "error.hpp"

namespace MWState
{
struct LoadGame : public Loading
{
    const Character *character{};
    std::string filepath;
    double &timePlayed;
    CharacterManager &characterManager;
    MWBase::StateManager::State &state;
    StateManager &stateManager;

    std::atomic_bool askForConfirmation{};
    std::shared_ptr<Choice> choice;
    bool firstPersonCam = false;

    LoadGame(double &out_timePlayed, CharacterManager &out_characterManager, MWBase::StateManager::State &out_state, StateManager &manager)
        : timePlayed(out_timePlayed), characterManager(out_characterManager), state(out_state), stateManager(manager)
    {
        mDescription = "#{sLoadingMessage14}";
    }

    std::map<int, int> buildContentFileIndexMap (const ESM::ESMReader& reader)
        const
    {
        const std::vector<std::string>& current =
            MWBase::Environment::get().getWorld()->getContentFiles();
        const std::vector<ESM::Header::MasterData>& prev = reader.getGameFiles();
        std::map<int, int> map;
        for (int iPrev = 0; iPrev<static_cast<int> (prev.size()); ++iPrev)
        {
            std::string id = Misc::StringUtils::lowerCase (prev[iPrev].name);
            for (int iCurrent = 0; iCurrent<static_cast<int> (current.size()); ++iCurrent)
                if (id==Misc::StringUtils::lowerCase (current[iCurrent]))
                {
                    map.insert (std::make_pair (iPrev, iCurrent));
                    break;
                }
        }
        return map;
    }

    void verifyProfile(const ESM::SavedGame& profile)
    {
        const std::vector<std::string>& selectedContentFiles = MWBase::Environment::get().getWorld()->getContentFiles();
        bool notFound = false;
        for (const std::string& contentFile : profile.mContentFiles)
        {
            if (std::find(selectedContentFiles.begin(), selectedContentFiles.end(), contentFile)
                    == selectedContentFiles.end())
            {
                Log(Debug::Warning) << "Warning: Saved game dependency " << contentFile << " is missing.";
                notFound = true;
            }
        }
        //if (notFound)
            askForConfirmation = true;
    }

    void threadLoad() override
    {
        Log(Debug::Info) << "Reading save file " << std::filesystem::path(filepath).filename().string();

        ESM::ESMReader reader;
        reader.open (filepath);

        if (reader.getFormat() > ESM::SavedGame::sCurrentFormat)
            throw std::runtime_error("This save file was created using a newer version of OpenMW and is thus not supported. Please upgrade to the newest OpenMW version to load this file.");

        std::map<int, int> contentFileMap = buildContentFileIndexMap (reader);
        MWBase::Environment::get().getLuaManager()->setContentFileMapping(contentFileMap);

        size_t total = reader.getFileSize();
        while (reader.hasMoreRecs() && !abort)
        {
            ESM::NAME n = reader.getRecName();
            reader.getRecHeader();

            switch (n.toInt())
            {
                case ESM::REC_SAVE:
                {
                    ESM::SavedGame profile;
                    profile.load(reader);
                    verifyProfile(profile);
                    timePlayed = profile.mTimePlayed;
                    Log(Debug::Info) << "Loading saved game '" << profile.mDescription << "' for character '" << profile.mPlayerName << "'";
                    break;
                }

                case ESM::REC_JOUR:
                case ESM::REC_JOUR_LEGACY:
                case ESM::REC_QUES:
                    MWBase::Environment::get().getJournal()->readRecord (reader, n.toInt());
                    break;

                case ESM::REC_DIAS:
                    MWBase::Environment::get().getDialogueManager()->readRecord (reader, n.toInt());
                    break;

                case ESM::REC_ALCH:
                case ESM::REC_ARMO:
                case ESM::REC_BOOK:
                case ESM::REC_CLAS:
                case ESM::REC_CLOT:
                case ESM::REC_ENCH:
                case ESM::REC_NPC_:
                case ESM::REC_SPEL:
                case ESM::REC_WEAP:
                case ESM::REC_GLOB:
                case ESM::REC_PLAY:
                case ESM::REC_CSTA:
                case ESM::REC_WTHR:
                case ESM::REC_DYNA:
                case ESM::REC_ACTC:
                case ESM::REC_PROJ:
                case ESM::REC_MPRJ:
                case ESM::REC_ENAB:
                case ESM::REC_LEVC:
                case ESM::REC_LEVI:
                case ESM::REC_CREA:
                case ESM::REC_CONT:
                case ESM::REC_RAND:
                    MWBase::Environment::get().getWorld()->readRecord(reader, n.toInt(), contentFileMap);
                    break;

                case ESM::REC_CAM_:
                    reader.getHNT(firstPersonCam, "FIRS");
                    break;

                case ESM::REC_GSCR:
                    MWBase::Environment::get().getScriptManager()->getGlobalScripts().readRecord (reader, n.toInt(), contentFileMap);
                    break;

                case ESM::REC_GMAP:
                case ESM::REC_KEYS:
                case ESM::REC_ASPL:
                case ESM::REC_MARK:
                    MWBase::Environment::get().getWindowManager()->readRecord(reader, n.toInt());
                    break;

                case ESM::REC_DCOU:
                case ESM::REC_STLN:
                    MWBase::Environment::get().getMechanicsManager()->readRecord(reader, n.toInt());
                    break;

                case ESM::REC_INPU:
                    MWBase::Environment::get().getInputManager()->readRecord(reader, n.toInt());
                    break;

                case ESM::REC_LUAM:
                    MWBase::Environment::get().getLuaManager()->readRecord(reader, n.toInt());
                    break;

                default:
                    // ignore invalid records
                    Log(Debug::Warning) << "Warning: Ignoring unknown record: " << n.toStringView();
                    reader.skipRecord();
            }
            setComplete(float(reader.getFileOffset())/total);
        }
    }

    void postLoad()
    {
        characterManager.setCurrentCharacter(character);

        state = StateManager::State_Running;

        if (character)
            Settings::Manager::setString ("character", "Saves", character->getPath().filename().string());

        MWBase::Environment::get().getWindowManager()->setNewGame(false);
        MWBase::Environment::get().getWorld()->saveLoaded();
        MWBase::Environment::get().getWorld()->setupPlayer();
        MWBase::Environment::get().getWorld()->renderPlayer();
        MWBase::Environment::get().getWindowManager()->updatePlayer();
        MWBase::Environment::get().getMechanicsManager()->playerLoaded();

        if (firstPersonCam != MWBase::Environment::get().getWorld()->isFirstPerson())
            MWBase::Environment::get().getWorld()->togglePOV();

        MWWorld::ConstPtr ptr = MWMechanics::getPlayer();

        //pushGameState(changeCell
        if (ptr.isInCell())
        {
            const ESM::CellId& cellId = ptr.getCell()->getCell()->getCellId();

            // Use detectWorldSpaceChange=false, otherwise some of the data we just loaded would be cleared again
            MWBase::Environment::get().getWorld()->changeToCell (cellId, ptr.getRefData().getPosition(), false, false);
        }
        else
        {
            // Cell no longer exists (i.e. changed game files), choose a default cell
            Log(Debug::Warning) << "Warning: Player character's cell no longer exists, changing to the default cell";
            MWWorld::CellStore* cell = MWBase::Environment::get().getWorld()->getExterior(0,0);
            float x,y;
            MWBase::Environment::get().getWorld()->indexToPosition(0,0,x,y,false);
            ESM::Position pos;
            pos.pos[0] = x;
            pos.pos[1] = y;
            pos.pos[2] = 0; // should be adjusted automatically (adjustPlayerPos=true)
            pos.rot[0] = 0;
            pos.rot[1] = 0;
            pos.rot[2] = 0;
            MWBase::Environment::get().getWorld()->changeToCell(cell->getCell()->getCellId(), pos, true, false);
        }

        MWBase::Environment::get().getWorld()->updateProjectilesCasters();

        // Vanilla MW will restart startup scripts when a save game is loaded. This is unintuitive,
        // but some mods may be using it as a reload detector.
        MWBase::Environment::get().getScriptManager()->getGlobalScripts().addStartup();

        // Since we passed "changeEvent=false" to changeCell, we shouldn't have triggered the cell change flag.
        // But make sure the flag is cleared anyway in case it was set from an earlier game.
        MWBase::Environment::get().getWorld()->markCellAsUnchanged();

        MWBase::Environment::get().getLuaManager()->gameLoaded();
    }
    void cleanup()
    {
        abort = true;
        if (mThread)
        {
            mThread->join();
            mThread = {};
        }
        stateManager.cleanup (true);
    }
    bool run(float dt) override
    {
        if (askForConfirmation && !abort)
        {
            if (!choice)
            {
                choice = std::make_shared<Choice>();
                choice->buttons = {"#{sYes}", "#{sNo}"};
                choice->message = "#{sMissingMastersMsg}";
                stateManager.pushGameState(choice);
                return true;
            }
            else if (choice->pressedButton != 0)
            {
                cleanup();
                stateManager.pushGameState(std::make_shared<Menu>());
                return false;
            }
        }

        try
        {
            auto threadRunning = Loading::run(dt);
            if (!threadRunning && !abort)
                postLoad();
            return threadRunning;
        }
        catch (const std::exception& e)
        {
            cleanup();
            std::stringstream error;
            error << "Failed to load saved game: " << e.what();
            stateManager.pushGameState(std::make_shared<Error>(error.str()));
            return false;
        }
    }
};
}
