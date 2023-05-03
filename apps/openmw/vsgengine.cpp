#include "vsgengine.hpp"

#include <vsgXchange/images.h>
#include <vsg/threading/OperationThreads.h>

#include <components/render/attachmentformat.hpp>
#include <components/render/screenshot.hpp>
#include <components/render/engine.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/sdlutil/init.hpp>
#include <components/version/version.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/registerarchives.hpp>
#include <components/vsgadapters/sdl/surface.hpp>
#include <components/vsgadapters/sdl/window.hpp>
#include <components/vsgutil/readimage.hpp>

#include "mwgui/windowmanagerimp.hpp"
#include "mwinput/inputmanagerimp.hpp"
#include "mwrender/rendermanager.hpp"
#include "mwsound/soundmanagerimp.hpp"
#include "mwstate/loadingscreen.hpp"
#include "mwstate/statemanagerimp.hpp"

#include "boot.hpp"

namespace
{
    void screenshot(Render::Engine& engine, vsg::ref_ptr<vsg::OperationThreads> threads,
        const std::filesystem::path& path, vsg::ref_ptr<const vsg::Options> writeOptions)
    {
        auto operation = engine.screenshotOperation();
        operation->writeOptions = writeOptions;
        operation->generateFilename(path, Settings::Manager::getString("screenshot format", "General"));
        threads->add(operation);
    }

    vsg::ref_ptr<vsg::Sampler> createDefaultSampler()
    {
        auto minFilter = VK_FILTER_LINEAR;
        auto magFilter = VK_FILTER_LINEAR;
        auto mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        auto magstr = Settings::Manager::getString("texture mag filter", "General");
        auto minstr = Settings::Manager::getString("texture min filter", "General");
        auto mipstr = Settings::Manager::getString("texture mipmap", "General");

        if (magstr == "nearest")
            magFilter = VK_FILTER_NEAREST;
        else if (magstr != "linear")
            Log(Debug::Warning) << "Warning: Invalid texture mag filter: " << magstr;

        if (minstr == "nearest")
            minFilter = VK_FILTER_NEAREST;
        else if (minstr != "linear")
            Log(Debug::Warning) << "Warning: Invalid texture min filter: " << minstr;

        if (mipstr == "nearest")
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        else if (mipstr != "linear")
            Log(Debug::Warning) << "Warning: Invalid texture mipmap: " << mipstr;

        auto sampler = vsg::Sampler::create();
        sampler->maxAnisotropy = Settings::Manager::getInt("anisotropy", "General");
        sampler->minFilter = minFilter;
        sampler->magFilter = magFilter;
        sampler->mipmapMode = mipmapMode;
        return sampler;
    }
}
namespace OMW
{
    struct WindowListener : public SDLUtil::WindowListener
    {
        Engine& mEngine;
        WindowListener(Engine& engine)
            : mEngine(engine)
        {
        }

        void windowVisibilityChange(bool visible)
        {
            mEngine.mWindowVisible = visible;
            if (!visible)
                mEngine.mSoundManager->pausePlayback();
            else
                mEngine.mSoundManager->resumePlayback();
        }
        void windowClosed() { mEngine.mStateManager->requestQuit(); }
        void windowResized(int x, int y)
        {
            Settings::Manager::setInt("resolution x", "Video", x);
            Settings::Manager::setInt("resolution y", "Video", y);
            mEngine.mWindowManager->windowResized(x, y);
            mEngine.mRenderManager->updateExtents(x, y);
        }
    };

    Engine::Engine(Files::ConfigurationManager& configurationManager)
        : mCfgMgr(configurationManager)
    {
        SDLUtil::init();
    }

    bool /*requiresRender*/ Engine::frame(float dt, std::shared_ptr<MWState::GameState>& gameState)
    {
        try
        {
            mInputManager->update(dt, gameState->disableControls, gameState->disableEvents);
        }
        catch (const std::exception& e)
        {
            std::cerr << "!InputManager::update(" << e.what() << ")" << std::endl;
        }
        if (mStateManager->getGameState() != gameState)
            return false;

        if (mInputManager->getScreenshotRequest())
            screenshot(*mRenderEngine, mRenderManager->getOperationThreads(), mCfgMgr.getScreenshotPath(), mResourceSystem->imageOptions);

        if (gameState->showCursor)
            mInputManager->enableMouse(*gameState->showCursor);

        if (!mWindowVisible)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return false;
        }
        mSoundManager->update(dt);

        try
        {
            if (!gameState->run(dt))
            {
                mStateManager->popGameState(gameState);
                return false;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "!GameState<" << typeid(*gameState).name() << ">::run(" << e.what() << ")" << std::endl;
            mStateManager->popGameState(gameState);
            return false;
        }
        mWindowManager->onFrame(dt);
        mRenderManager->setViewMode(MWRender::ViewMode::Scene, gameState->requiresScene);
        if (gameState->requiresScene)
            mRenderManager->onFrame(dt);
        return true;
    }

    Engine::~Engine()
    {
        if (mWindow)
        {
            SDL_DestroyWindow(mWindow);
            mWindow = nullptr;
        }

        SDL_Quit();
    }

    void Engine::createWindow()
    {
        int screen = Settings::Manager::getInt("screen", "Video");
        int width = Settings::Manager::getInt("resolution x", "Video");
        int height = Settings::Manager::getInt("resolution y", "Video");
        bool fullscreen = Settings::Manager::getInt("window mode", "Video") != 2;
        bool windowBorder = Settings::Manager::getBool("window border", "Video");
        bool vsync = Settings::Manager::getBool("vsync", "Video");
        unsigned int antialiasing = std::max(0, Settings::Manager::getInt("antialiasing", "Video"));

        int pos_x = SDL_WINDOWPOS_CENTERED_DISPLAY(screen), pos_y = SDL_WINDOWPOS_CENTERED_DISPLAY(screen);

        if (fullscreen)
        {
            pos_x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(screen);
            pos_y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(screen);
        }

        bool indirect = getenv("VSGOPENMW_INDIRECT") != 0;

        Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE; // SDL_WINDOW_ALLOWHIGHDPI
        if (!indirect)
            flags |= SDL_WINDOW_VULKAN;
        if (fullscreen)
            flags |= SDL_WINDOW_FULLSCREEN;

        if (!windowBorder)
            flags |= SDL_WINDOW_BORDERLESS;

        SDL_SetHint(
            SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, Settings::Manager::getBool("minimize on focus loss", "Video") ? "1" : "0");

        mWindow = SDL_CreateWindow("OpenMW", pos_x, pos_y, width, height, flags);
        if (!mWindow)
            throw std::runtime_error(SDL_GetError());

#if 0
    // Since we /*vsgopenmw-fixme*/ use physical resolution internally, we have to create the window with scaled resolution,
    // but we can't get the scale before the window exists, so instead we have to resize aftewards.
   int w,h;
   SDL_GetWindowSize(mWindow, &w, &h);
   int dw,dh;
   SDL_GL_GetDrawableSize(mWindow, &dw, &dh);
   if (dw != w || dh != h)
       SDL_SetWindowSize(mWindow, width / (dw / w), height / (dh / h));
#endif

        auto traits = vsg::WindowTraits::create();
        traits->width = width;
        traits->height = height;
        traits->debugLayer = getenv("VSGOPENMW_VALIDATION") != 0;
        traits->queueFlags |= VK_QUEUE_COMPUTE_BIT;
        traits->depthFormat = Render::compatibleDepthFormat;
        traits->swapchainPreferences.presentMode = vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        traits->swapchainPreferences.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // enableScreenshot
        traits->depthImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // enableReflectionDepth
        traits->swapchainPreferences.surfaceFormat.format = Render::compatibleColorFormat;
        if (antialiasing)
            traits->samples = antialiasing;

        if (!indirect)
        {
            vsg::ref_ptr<vsg::WindowAdapter> adapter(new vsgAdapters::sdl::Window(mWindow, traits));
            mRenderEngine = std::make_unique<Render::Engine>(adapter);
        }
        else
        {
            auto callback = [this](vsg::ref_ptr<vsg::Data> data) {
                vsgAdapters::sdl::fillSurface(SDL_GetWindowSurface(mWindow), data);
                SDL_UpdateWindowSurface(mWindow);
            };
            mRenderEngine = std::make_unique<Render::Engine>(callback, traits);
        }
    }

    void Engine::setWindowIcon(const std::filesystem::path& file)
    {
        auto options = vsg::Options::create();
        options->readerWriters = { vsgXchange::images::create() };
        if (auto image = vsgUtil::readOptionalImage(file, options))
        {
            if (auto surface = vsgAdapters::sdl::createSurface(image))
                SDL_SetWindowIcon(mWindow, surface.get());
        }
    }

    void Engine::prepareEngine(const Arguments& args)
    {
        createWindow();

        Misc::Rng::init(args.randomSeed);

        mVFS = std::make_unique<VFS::Manager>(args.fsStrict);

        mDataDirs = args.dataDirs;
        mCfgMgr.filterOutNonExistingPaths(mDataDirs);
        mDataDirs.insert(mDataDirs.begin(), args.resourceDir / "vfs");
        mFileCollections = Files::Collections(mDataDirs, !args.fsStrict);
        VFS::registerArchives(mVFS.get(), mFileCollections, args.archives, true);

        mResourceSystem = std::make_unique<Resource::ResourceSystem>(mVFS.get(), args.resourceDir / "shaders",
            createDefaultSampler(), mRenderEngine->supportsCompressedImages());

        setWindowIcon(args.resourceDir / "openmw.png");

        // vsgopenmw-fixme(global-state)
        mEnvironment.setResourceSystem(*mResourceSystem);

        // Create input and UI first to set up a bootstrapping environment for
        // showing a loading screen and keeping the window responsive while doing so

        auto keybinderUser = mCfgMgr.getUserConfigPath() / "input_v3.xml";
        bool keybinderUserExists = std::filesystem::exists(keybinderUser);
        if (!keybinderUserExists)
        {
            auto input2 = mCfgMgr.getUserConfigPath() / "input_v2.xml";
            if (std::filesystem::exists(input2))
            {
                std::filesystem::copy_file(input2, keybinderUser);
                keybinderUserExists = std::filesystem::exists(keybinderUser);
                std::cout << "Loading keybindings file: " << keybinderUser << std::endl;
            }
        }
        else
            std::cout << "Loading keybindings file: " << keybinderUser << std::endl;

        auto userdefault = mCfgMgr.getUserConfigPath() / "gamecontrollerdb.txt";
        auto localdefault = mCfgMgr.getLocalPath() / "gamecontrollerdb.txt";
        auto globaldefault = mCfgMgr.getGlobalPath() / "gamecontrollerdb.txt";

        std::filesystem::path userGameControllerdb;
        if (std::filesystem::exists(userdefault))
        {
            userGameControllerdb = userdefault;
        }
        else
            userGameControllerdb = "";

        std::filesystem::path gameControllerdb;
        if (std::filesystem::exists(localdefault))
            gameControllerdb = localdefault;
        else if (std::filesystem::exists(globaldefault))
            gameControllerdb = globaldefault;

        mEncoder = std::make_unique<ToUTF8::Utf8Encoder>(args.encoding);
        mTranslationDataStorage.setEncoder(mEncoder.get());
        for (auto& f : args.contentFiles)
            mTranslationDataStorage.loadTranslationData(mFileCollections, f);

        mL10nManager = std::make_unique<l10n::Manager>(mVFS.get());
        mL10nManager->setPreferredLocales(Settings::Manager::getStringArray("preferred locales", "General"));
        mEnvironment.setL10nManager(*mL10nManager);

        auto compile = mRenderEngine->createCompileContext();

        mWindowManager = std::make_unique<MWGui::WindowManager>(mWindow, compile, mResourceSystem.get(),
            mCfgMgr.getLogPath(), args.scriptConsoleMode, mTranslationDataStorage, args.encoding,
            Version::getOpenmwVersionDescription(args.resourceDir), mCfgMgr);
        mEnvironment.setWindowManager(*mWindowManager);

        mInputManager = std::make_unique<MWInput::InputManager>(
            mWindow, keybinderUser, keybinderUserExists, userGameControllerdb, gameControllerdb, args.grabMouse);
        mWindowListener = std::make_unique<WindowListener>(*this);
        mInputManager->setWindowListener(mWindowListener.get());
        mEnvironment.setInputManager(*mInputManager);

        mRenderManager = std::make_unique<MWRender::RenderManager>(*mRenderEngine, mWindowManager->node(),
            mResourceSystem.get() /*, *mNavigator*/);

        mSoundManager = std::make_unique<MWSound::SoundManager>(mVFS.get(), args.useSound);
        mEnvironment.setSoundManager(*mSoundManager);

        mStateManager = std::make_unique<MWState::StateManager>(
            mCfgMgr.getUserDataPath() / "saves", args.contentFiles, mRenderManager->getScreenshotInterface());
        mEnvironment.setStateManager(*mStateManager);

        auto loadingScreen = mWindowManager->createLoadingScreen();
        loadingScreen->loading = std::make_shared<Boot>(*this, args);
        mStateManager->pushGameState(loadingScreen);
    }

    void Engine::go(const Arguments& args)
    {
        try
        {
            prepareEngine(args);
            mRenderEngine->compile();
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            return;
        }

        auto callback = [this](double dt, bool& requiresRender, bool& quit) {
            auto state = mStateManager->getGameState();
            quit = !state;
            if (quit)
                return;
            requiresRender = frame(dt, state);
        };
        mRenderEngine->loop(callback, Settings::Manager::getFloat("framerate limit", "Video"));

        std::cout << "Quitting peacefully." << std::endl;

        Settings::Manager::saveUser(mCfgMgr.getUserConfigPath() / "settings.cfg");
    }
}
