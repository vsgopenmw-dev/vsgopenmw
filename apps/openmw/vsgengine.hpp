#ifndef VSGOPENMW_ENGINE_H
#define VSGOPENMW_ENGINE_H

#include <vsg/core/ref_ptr.h>

#include <components/compiler/extensions.hpp>
#include <components/files/collections.hpp>
#include <components/translation/translation.hpp>
#include <components/settings/settings.hpp>
#include <components/render/engine.hpp>

#include "mwbase/environment.hpp"

namespace Resource
{
    class ResourceSystem;
}
namespace vsg
{
    class OperationThreads;
}
namespace SceneUtil
{
    class WorkQueue;
}

namespace VFS
{
    class Manager;
}

namespace Compiler
{
    class Context;
}

namespace MWLua
{
    class LuaManager;
}

namespace Files
{
    struct ConfigurationManager;
}

namespace MWState
{
    class StateManager;
    class GameState;
}
namespace MWGui
{
    class WindowManager;
}
namespace MWInput
{
    class InputManager;
}
namespace MWSound
{
    class SoundManager;
}
namespace MWRender
{
    class RenderManager;
}
namespace MWWorld
{
    class World;
}
namespace MWScript
{
    class ScriptManager;
}
namespace MWMechanics
{
    class MechanicsManager;
}
namespace MWDialogue
{
    class DialogueManager;
}
namespace MWDialogue
{
    class Journal;
}

struct SDL_Window;

namespace OMW
{
    class WindowListener;
    /*
     * Runs game states.
     */
    class Engine
    {
        friend class Boot;
        friend class Game;

        SDL_Window* mWindow{};
        std::unique_ptr<VFS::Manager> mVFS;
        std::unique_ptr<ToUTF8::Utf8Encoder> mEncoder;
        std::unique_ptr<Resource::ResourceSystem> mResourceSystem;

        std::unique_ptr<Compiler::Context> mScriptContext;
        std::unique_ptr<MWLua::LuaManager> mLuaManager;
        std::unique_ptr<MWState::StateManager> mStateManager;
        std::unique_ptr<MWInput::InputManager> mInputManager;
        std::unique_ptr<MWSound::SoundManager> mSoundManager;
        std::unique_ptr<MWRender::RenderManager> mRenderManager;
        std::unique_ptr<MWWorld::World> mWorld;
        std::unique_ptr<MWGui::WindowManager> mWindowManager;
        std::unique_ptr<MWScript::ScriptManager> mScriptManager;
        std::unique_ptr<MWDialogue::Journal> mJournal;
        std::unique_ptr<MWDialogue::DialogueManager> mDialogueManager;
        std::unique_ptr<MWMechanics::MechanicsManager> mMechanicsManager;

        MWBase::Environment mEnvironment;
        ToUTF8::FromType mEncoding;
        Files::PathContainer mDataDirs;
        std::vector<std::string> mArchives;
        boost::filesystem::path mResDir;
        Render::Engine mRenderEngine;
        vsg::ref_ptr<vsg::OperationThreads> mThreads;
        std::string mCellName;
        std::vector<std::string> mContentFiles;
        std::vector<std::string> mGroundcoverFiles;
        bool mSkipMenu = false;
        bool mUseSound = true;
        bool mCompileAll = false;
        bool mCompileAllDialogue = false;
        int mWarningsMode = 1;
        std::string mFocusName;
        bool mScriptConsoleMode = false;
        std::string mStartupScript;
        int mActivationDistanceOverride = -1;
        std::string mSaveGameFile;
        // Grab mouse?
        bool mGrab = true;

        unsigned int mRandomSeed = 0;

        Compiler::Extensions mExtensions;

        Files::Collections mFileCollections;
        bool mFSStrict = false;
        Translation::Storage mTranslationDataStorage;
        std::vector<std::string> mScriptBlacklist;
        bool mScriptBlacklistUse = true;
        bool mNewGame = false;

        bool mWindowVisible = true;
        std::unique_ptr<WindowListener> mWindowListener;

        // not implemented
        Engine (const Engine&);
        Engine& operator= (const Engine&);

        bool frame (float dt, std::shared_ptr<MWState::GameState> &state);

        /// Load settings from various files, returns the path to the user settings file
        std::string loadSettings (Settings::Manager & settings);

        void prepareEngine (Settings::Manager & settings);

        void createWindow(Settings::Manager& settings);
        void setWindowIcon();

    public:
        Engine(Files::ConfigurationManager& configurationManager);
        virtual ~Engine();

        //vsgopenmw-fixme(add-public-access)

        /// Enable strict filesystem mode (do not fold case)
        ///
        /// \attention The strict mode must be specified before any path-related settings
        /// are given to the engine.
        void enableFSStrict(bool fsStrict);

        /// Set data dirs
        void setDataDirs(const Files::PathContainer& dataDirs);

        /// Add BSA archive
        void addArchive(const std::string& archive);

        /// Set resource dir
        void setResourceDir(const boost::filesystem::path& parResDir);

        /// Set start cell name
        void setCell(const std::string& cellName);

        /**
         * @brief addContentFile - Adds content file (ie. esm/esp, or omwgame/omwaddon) to the content files container.
         * @param file - filename (extension is required)
         */
        void addContentFile(const std::string& file);
        void addGroundcoverFile(const std::string& file);

        /// Disable or enable all sounds
        void setSoundUsage(bool soundUsage);

        /// Skip main menu and go directly into the game
        ///
        /// \param newGame Start a new game instead off dumping the player into the game
        /// (ignored if !skipMenu).
        void setSkipMenu (bool skipMenu, bool newGame);

        void setGrabMouse(bool grab) { mGrab = grab; }

        /// Initialise and enter main loop.
        void go();

        /// Compile all scripts (excludign dialogue scripts) at startup?
        void setCompileAll (bool all);

        /// Compile all dialogue scripts at startup?
        void setCompileAllDialogue (bool all);

        /// Font encoding
        void setEncoding(const ToUTF8::FromType& encoding);

        /// Enable console-only script functionality
        void setScriptConsoleMode (bool enabled);

        /// Set path for a script that is run on startup in the console.
        void setStartupScript (const std::string& path);

        /// Override the game setting specified activation distance.
        void setActivationDistanceOverride (int distance);

        void setWarningsMode (int mode);

        void setScriptBlacklist (const std::vector<std::string>& list);

        void setScriptBlacklistUse (bool use);

        /// Set the save game file to load after initialising the engine.
        void setSaveGameFile(const std::string& savegame);

        void setRandomSeed(unsigned int seed);

        friend class WindowListener;
    private:
        Files::ConfigurationManager& mCfgMgr;
    };
}

#endif
