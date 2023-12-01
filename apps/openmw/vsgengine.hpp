#ifndef VSGOPENMW_ENGINE_H
#define VSGOPENMW_ENGINE_H

#include <vsg/core/ref_ptr.h>

#include <components/files/collections.hpp>
#include <components/translation/translation.hpp>

#include "mwbase/environment.hpp"

#include "arguments.hpp"

namespace Resource
{
    class ResourceSystem;
}
namespace VFS
{
    class Manager;
}

namespace l10n
{
    class Manager;
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
namespace Render
{
    class Engine;
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
        friend class WindowListener;

        std::unique_ptr<Render::Engine> mRenderEngine;
        SDL_Window* mWindow{};
        std::unique_ptr<VFS::Manager> mVFS;
        std::unique_ptr<Resource::ResourceSystem> mResourceSystem;
        std::unique_ptr<MWInput::InputManager> mInputManager;
        std::unique_ptr<MWSound::SoundManager> mSoundManager;
        std::unique_ptr<MWRender::RenderManager> mRenderManager;
        std::unique_ptr<MWGui::WindowManager> mWindowManager;
        std::unique_ptr<MWState::StateManager> mStateManager;
        std::unique_ptr<l10n::Manager> mL10nManager;
        MWBase::Environment mEnvironment;
        std::unique_ptr<ToUTF8::Utf8Encoder> mEncoder;
        Translation::Storage mTranslationDataStorage;
        Files::PathContainer mDataDirs;
        Files::Collections mFileCollections;
        Files::ConfigurationManager& mCfgMgr;

        bool mWindowVisible = true;
        std::unique_ptr<WindowListener> mWindowListener;

        // not implemented
        Engine(const Engine&);
        Engine& operator=(const Engine&);

        bool frame(float dt, std::shared_ptr<MWState::GameState>& state);

        void prepareEngine(const Arguments& args);

        void createRenderEngine();
        void setWindowIcon(const std::filesystem::path& file);

    public:
        Engine(Files::ConfigurationManager& configurationManager);
        ~Engine();

        /// Initialise and enter main loop.
        void go(const Arguments& args);
    };
}

#endif
