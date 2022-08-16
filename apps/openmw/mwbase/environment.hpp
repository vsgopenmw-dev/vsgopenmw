#ifndef GAME_BASE_ENVIRONMENT_H
#define GAME_BASE_ENVIRONMENT_H

#include <memory>

namespace osg
{
    class Stats;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWBase
{
    class World;
    class ScriptManager;
    class DialogueManager;
    class Journal;
    class SoundManager;
    class MechanicsManager;
    class InputManager;
    class WindowManager;
    class StateManager;
    class LuaManager;

    /// \brief Central hub for mw-subsystems
    ///
    /// This class allows each mw-subsystem to access any others subsystem's top-level manager class.
    ///
    class Environment
    {
            static Environment *sThis;

            World* mWorld = nullptr;
            SoundManager* mSoundManager = nullptr;
            ScriptManager* mScriptManager = nullptr;
            WindowManager* mWindowManager = nullptr;
            MechanicsManager* mMechanicsManager = nullptr;
            DialogueManager* mDialogueManager = nullptr;
            Journal* mJournal = nullptr;
            InputManager* mInputManager = nullptr;
            StateManager* mStateManager = nullptr;
            LuaManager* mLuaManager = nullptr;
            Resource::ResourceSystem* mResourceSystem = nullptr;
            float mFrameDuration{};

        public:

            Environment();

            ~Environment();

            Environment(const Environment&) = delete;

            Environment& operator=(const Environment&) = delete;

            void setWorld(World& value) { mWorld = &value; }

            void setSoundManager(SoundManager& value) { mSoundManager = &value; }

            void setScriptManager(ScriptManager& value) { mScriptManager = &value; }

            void setWindowManager(WindowManager& value) { mWindowManager = &value; }

            void setMechanicsManager(MechanicsManager& value) { mMechanicsManager = &value; }

            void setDialogueManager(DialogueManager& value) { mDialogueManager = &value; }

            void setJournal(Journal& value) { mJournal = &value; }

            void setInputManager(InputManager& value) { mInputManager = &value; }

            void setStateManager(StateManager& value) { mStateManager = &value; }

            void setLuaManager(LuaManager& value) { mLuaManager = &value; }

            void setResourceSystem(Resource::ResourceSystem& value) { mResourceSystem = &value; }

            World *getWorld() const { return mWorld; }

            SoundManager *getSoundManager() const { return mSoundManager; }

            ScriptManager *getScriptManager() const { return mScriptManager; }

            WindowManager *getWindowManager() const { return mWindowManager; }

            MechanicsManager *getMechanicsManager() const { return mMechanicsManager; }

            DialogueManager *getDialogueManager() const { return mDialogueManager; }

            Journal *getJournal() const { return mJournal; }

            InputManager *getInputManager() const { return mInputManager; }

            StateManager *getStateManager() const { return mStateManager; }

            LuaManager *getLuaManager() const { return mLuaManager; }

            //vsgopenmw-fixme(global-state)
            Resource::ResourceSystem *getResourceSystem() const { return mResourceSystem; }

            //vsgopenmw-fixme(global-state)
            float getFrameDuration() const { return mFrameDuration; }

            //vsgopenmw-fixme(global-state)
            void setFrameDuration(float value) { mFrameDuration = value; }

            /// Return instance of this class.
            static const Environment& get()
            {
                return *sThis;
            }

            void reportStats(unsigned int frameNumber, osg::Stats& stats) const;
    };
}

#endif
