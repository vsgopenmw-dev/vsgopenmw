//vsgopenmw-unity-build

#include <sstream>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>

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
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwscript/globalscripts.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/settings/settings.hpp>

#include "loading.hpp"
#include "character.hpp"

namespace MWState
{
struct SaveGame : public Loading
{
    std::string description;
    const Slot *slot{};
    Character *character{};
    double timePlayed{};
    SaveGame()
    {
        mDescription = "#{sNotifyMessage4}";
    }
    /*
void MWState::StateManager::writeScreenshot(std::vector<char> &imageData) const
{
    int screenshotW = 259*2, screenshotH = 133*2; // *2 to get some nice antialiasing

    MWBase::Environment::get().getWorld()->screenshot(screenshot.get(), screenshotW, screenshotH);
}
*/
    void threadLoad() override
    {
        ESM::SavedGame profile;

        MWBase::World& world = *MWBase::Environment::get().getWorld();

        MWWorld::Ptr player = world.getPlayerPtr();

        profile.mContentFiles = world.getContentFiles();

        profile.mPlayerName = player.get<ESM::NPC>()->mBase->mName;
        profile.mPlayerLevel = player.getClass().getNpcStats (player).getLevel();

        std::string classId = player.get<ESM::NPC>()->mBase->mClass;
        if (world.getStore().get<ESM::Class>().isDynamic(classId))
            profile.mPlayerClassName = world.getStore().get<ESM::Class>().find(classId)->mName;
        else
            profile.mPlayerClassId = classId;

        profile.mPlayerCell = world.getCellName();
        profile.mInGameTime = world.getEpochTimeStamp();
        profile.mTimePlayed = timePlayed;
        profile.mDescription = description;

        //writeScreenshot(profile.mScreenshot);

        if (!slot)
            slot = character->createSlot (profile);
        else
            slot = character->updateSlot (slot, profile);

        // Make sure the animation state held by references is up to date before saving the game.
        MWBase::Environment::get().getMechanicsManager()->persistAnimationStates();

        Log(Debug::Info) << "Writing saved game '" << description << "' for character '" << profile.mPlayerName << "'";

        // Write to a memory stream first. If there is an exception during the save process, we don't want to trash the
        // existing save file we are overwriting.
        std::stringstream stream;

        ESM::ESMWriter writer;

        for (const std::string& contentFile : MWBase::Environment::get().getWorld()->getContentFiles())
            writer.addMaster(contentFile, 0); // not using the size information anyway -> use value of 0

        writer.setFormat (ESM::SavedGame::sCurrentFormat);

        // all unused
        writer.setVersion(0);
        writer.setType(0);
        writer.setAuthor("");
        writer.setDescription("");

        int recordCount =         1 // saved game header
                +MWBase::Environment::get().getJournal()->countSavedGameRecords()
                +MWBase::Environment::get().getLuaManager()->countSavedGameRecords()
                +MWBase::Environment::get().getWorld()->countSavedGameRecords()
                +MWBase::Environment::get().getScriptManager()->getGlobalScripts().countSavedGameRecords()
                +MWBase::Environment::get().getDialogueManager()->countSavedGameRecords()
                +MWBase::Environment::get().getMechanicsManager()->countSavedGameRecords()
                +MWBase::Environment::get().getInputManager()->countSavedGameRecords()
                +MWBase::Environment::get().getWindowManager()->countSavedGameRecords();
        writer.setRecordCount (recordCount);

        writer.save (stream);

        ::Loading::Listener /*dummy*/listener;
        // Using only Cells for progress information, since they typically have the largest records by far
        progressStep = 1.f/MWBase::Environment::get().getWorld()->countSavedGameCells();

        writer.startRecord (ESM::REC_SAVE);
        slot->mProfile.save (writer);
        writer.endRecord (ESM::REC_SAVE);

        MWBase::Environment::get().getJournal()->write (writer, listener);
        MWBase::Environment::get().getDialogueManager()->write (writer, listener);
        // LuaManager::write should be called before World::write because world also saves
        // local scripts that depend on LuaManager.
        MWBase::Environment::get().getLuaManager()->write(writer, listener);
        MWBase::Environment::get().getWorld()->write (writer, *this);
        MWBase::Environment::get().getScriptManager()->getGlobalScripts().write (writer, listener);
        MWBase::Environment::get().getMechanicsManager()->write(writer, listener);
        MWBase::Environment::get().getInputManager()->write(writer, listener);
        MWBase::Environment::get().getWindowManager()->write(writer, listener);

        // Ensure we have written the number of records that was estimated
        if (writer.getRecordCount() != recordCount+1) // 1 extra for TES3 record
            Log(Debug::Warning) << "Warning: number of written savegame records does not match. Estimated: " << recordCount+1 << ", written: " << writer.getRecordCount();

        writer.close();

        if (stream.fail())
            throw std::runtime_error("Write operation failed (memory stream)");

        // All good, write to file
        boost::filesystem::ofstream filestream (slot->mPath, std::ios::binary);
        filestream << stream.rdbuf();

        if (filestream.fail())
            throw std::runtime_error("Write operation failed (file stream)");

        Settings::Manager::setString ("character", "Saves",
            slot->mPath.parent_path().filename().string());
    }
    bool run(float dt) override
    {
        try
        {
            return Loading::run(dt);
        }
        catch (const std::exception& e)
        {
            std::stringstream error;
            error << "Failed to save game: " << e.what();
            Log(Debug::Error) << error.str();

            std::vector<std::string> buttons;
            buttons.emplace_back("#{sOk}");
            MWBase::Environment::get().getWindowManager()->interactiveMessageBox(error.str(), buttons);

            // If no file was written, clean up the slot
            if (character && slot && !boost::filesystem::exists(slot->mPath))
            {
                character->deleteSlot(slot);
                character->cleanup();
            }
            return false;
        }
    }
};
}
