#include "vsgengine.hpp"

#include <vsg/vk/Context.h>
#include <vsg/viewer/CompileManager.h>
#include <vsg/threading/OperationThreads.h>

#include <SDL.h>

#include <components/misc/rng.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/registerarchives.hpp>
#include <components/vsgadapters/sdl/window.hpp>
#include <components/vsgadapters/sdl/surface.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/version/version.hpp>
#include <components/render/screenshot.hpp>
#include <components/render/attachmentformat.hpp>
#include <components/vsgutil/readimage.hpp>

#include "mwinput/inputmanagerimp.hpp"
#include "mwgui/windowmanagerimp.hpp"
#include "mwlua/luamanagerimp.hpp"
#include "mwsound/soundmanagerimp.hpp"
#include "mwrender/rendermanager.hpp"
#include "mwworld/worldimp.hpp"
#include "mwstate/statemanagerimp.hpp"
#include "mwstate/loadingscreen.hpp"

#include "boot.hpp"

namespace
{
    void screenshot(Render::Engine &engine, vsg::ref_ptr<vsg::OperationThreads> threads, const std::string &path, vsg::ref_ptr<const vsg::Options> writeOptions)
    {
        auto operation = engine.screenshotOperation();
        operation->writeOptions = writeOptions;
        operation->generateFilename(path, Settings::Manager::getString("screenshot format", "General"));
        threads->add(operation);
    }
}
namespace OMW
{
struct WindowListener : public SDLUtil::WindowListener
{
    Engine &mEngine;
    WindowListener(Engine &engine) : mEngine(engine) {}

    void windowVisibilityChange( bool visible )
    {
        mEngine.mWindowVisible = true;
    }
    void windowClosed ()
    {
        mEngine.mStateManager->requestQuit();
    }
    void windowResized (int x, int y)
    {
        Settings::Manager::setInt("resolution x", "Video", x);
        Settings::Manager::setInt("resolution y", "Video", y);
        mEngine.mWindowManager->windowResized(x, y);
        mEngine.mRenderManager->updateExtents(x, y);
    }
};

Engine::Engine(Files::ConfigurationManager& configurationManager)
  : mEncoding(ToUTF8::WINDOWS_1252)
  , mCfgMgr(configurationManager)
{
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0"); // We use only gamepads
    if (getenv("VSGOPENMW_INDIRECT") != 0)
    {
        SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    }

    Uint32 flags = SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE|SDL_INIT_GAMECONTROLLER|SDL_INIT_JOYSTICK|SDL_INIT_SENSOR;
    if(SDL_WasInit(flags) == 0)
    {
        SDL_SetMainReady();
        if(SDL_Init(flags) != 0)
        {
            throw std::runtime_error("Could not initialize SDL! " + std::string(SDL_GetError()));
        }
    }
}

bool /*requiresRender*/ Engine::frame(float dt, std::shared_ptr<MWState::GameState> &gameState)
{
    try
    {
        mEnvironment.setFrameDuration(dt);

        mInputManager->update(dt, gameState->disableControls, gameState->disableEvents);
        if (mStateManager->getGameState() != gameState)
            return false;

        if (mInputManager->getScreenshotRequest())
            screenshot(mRenderEngine, mThreads, mCfgMgr.getScreenshotPath().string(), mResourceSystem->imageOptions);

        bool cursor = gameState->showCursor;
        mWindowManager->setCursorVisible(cursor);
        mInputManager->enableMouse(cursor);

        if (!mWindowVisible)
        {
            mSoundManager->pausePlayback();
            return false;
        }
        else
            mSoundManager->resumePlayback();
        mSoundManager->update(dt);

        mRenderManager->setCompileRequired(!gameState->requiresCompile);

        bool keepRunning = gameState->run(dt);
        if (!keepRunning)
        {
            if (gameState->requiresCompile)
                mRenderEngine.compile();
            mStateManager->popGameState(gameState);
            return false;
        }
        mRenderManager->setViewMode(MWRender::ViewMode::Scene, gameState->requiresScene);
        mRenderManager->onFrame(dt);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error in frame: " << e.what() << std::endl;
    }
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

void Engine::enableFSStrict(bool fsStrict)
{
    mFSStrict = fsStrict;
}

void Engine::setDataDirs (const Files::PathContainer& dataDirs)
{
    mDataDirs = dataDirs;
    mDataDirs.insert(mDataDirs.begin(), (mResDir / "vfs"));
    mFileCollections = Files::Collections (mDataDirs, !mFSStrict);
}

void Engine::addArchive (const std::string& archive) {
    mArchives.push_back(archive);
}

void Engine::setResourceDir (const boost::filesystem::path& parResDir)
{
    mResDir = parResDir;
}

void Engine::setCell (const std::string& cellName)
{
    mCellName = cellName;
}

void Engine::addContentFile(const std::string& file)
{
    mContentFiles.push_back(file);
}

void Engine::addGroundcoverFile(const std::string& file)
{
    mGroundcoverFiles.emplace_back(file);
}

void Engine::setSkipMenu (bool skipMenu, bool newGame)
{
    mSkipMenu = skipMenu;
    mNewGame = newGame;
}

void Engine::createWindow(Settings::Manager& settings)
{
    int screen = settings.getInt("screen", "Video");
    int width = settings.getInt("resolution x", "Video");
    int height = settings.getInt("resolution y", "Video");
    bool fullscreen = settings.getInt("window mode", "Video") != 2;
    bool windowBorder = settings.getBool("window border", "Video");
    bool vsync = settings.getBool("vsync", "Video");
    unsigned int antialiasing = std::max(0, settings.getInt("antialiasing", "Video"));

    int pos_x = SDL_WINDOWPOS_CENTERED_DISPLAY(screen),
        pos_y = SDL_WINDOWPOS_CENTERED_DISPLAY(screen);

    if(fullscreen)
    {
        pos_x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(screen);
        pos_y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(screen);
    }

    bool indirect = getenv("VSGOPENMW_INDIRECT") != 0;

    Uint32 flags = SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE;
    if (!indirect)
        flags |= SDL_WINDOW_VULKAN;
    if(fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;

    // Allows for Windows snapping features to properly work in borderless window
    SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "1");
    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");

    if (!windowBorder)
        flags |= SDL_WINDOW_BORDERLESS;

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS,
                settings.getBool("minimize on focus loss", "Video") ? "1" : "0");

    mWindow = SDL_CreateWindow("OpenMW", pos_x, pos_y, width, height, flags);
    if (!mWindow)
        throw std::runtime_error(SDL_GetError());

    auto traits = vsg::WindowTraits::create();
    traits->width = width;
    traits->height = height;
    traits->debugLayer = getenv("VSGOPENMW_VALIDATION") != 0;
    traits->queueFlags |= VK_QUEUE_COMPUTE_BIT;
    traits->depthFormat = Render::compatibleDepthFormat;
    traits->swapchainPreferences.presentMode = vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    traits->swapchainPreferences.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; //enableScreenshot
    traits->swapchainPreferences.surfaceFormat.format = Render::compatibleColorFormat;
    if (antialiasing)
        traits->samples = antialiasing;
    if (!indirect)
    {
        vsg::ref_ptr<vsg::WindowAdapter> adapter(new vsgAdapters::sdl::Window(mWindow, traits));
        mRenderEngine.setWindow(adapter);
    }
    else
    {
        auto callback = [this](vsg::ref_ptr<vsg::Data> data) {
            vsgAdapters::sdl::fillSurface(SDL_GetWindowSurface(mWindow), data, false);
            SDL_UpdateWindowSurface(mWindow);
        };
        mRenderEngine.setHeadless(callback, traits);
    }
}

void Engine::setWindowIcon()
{
    auto image = vsgUtil::readImage((mResDir / "openmw.png").string(), mResourceSystem->imageOptions);
    SDL_SetWindowIcon(mWindow, vsgAdapters::sdl::createSurface(image).get());
}

void Engine::prepareEngine (Settings::Manager & settings)
{
    mStateManager = std::make_unique<MWState::StateManager> (mCfgMgr.getUserDataPath() / "saves", mContentFiles);
    mEnvironment.setStateManager(*mStateManager);

    createWindow(settings);

    mVFS = std::make_unique<VFS::Manager>(mFSStrict);

    VFS::registerArchives(mVFS.get(), mFileCollections, mArchives, true);

    mResourceSystem = std::make_unique<Resource::ResourceSystem>(mVFS.get(), (mResDir / "shaders").string());

    setWindowIcon();

    //vsgopenmw-fixme(global-state)
    mEnvironment.setResourceSystem(*mResourceSystem);
    /*
    mResourceSystem->getSceneManager()->setFilterSettings(
        Settings::Manager::getString("texture mag filter", "General"),
        Settings::Manager::getString("texture min filter", "General"),
        Settings::Manager::getString("texture mipmap", "General"),
        Settings::Manager::getInt("anisotropy", "General")
    );
    */

    int numThreads = Settings::Manager::getInt("preload num threads", "Cells");
    if (numThreads <= 0)
        throw std::runtime_error("Invalid setting: 'preload num threads' must be >0");
    mThreads = vsg::OperationThreads::create(numThreads);

    mLuaManager = std::make_unique<MWLua::LuaManager>(mVFS.get(), (mResDir / "lua_libs").string());
    mEnvironment.setLuaManager(*mLuaManager);

    // Create input and UI first to set up a bootstrapping environment for
    // showing a loading screen and keeping the window responsive while doing so

    std::string keybinderUser = (mCfgMgr.getUserConfigPath() / "input_v3.xml").string();
    bool keybinderUserExists = boost::filesystem::exists(keybinderUser);
    if(!keybinderUserExists)
    {
        std::string input2 = (mCfgMgr.getUserConfigPath() / "input_v2.xml").string();
        if(boost::filesystem::exists(input2)) {
            boost::filesystem::copy_file(input2, keybinderUser);
            keybinderUserExists = boost::filesystem::exists(keybinderUser);
            std::cout << "Loading keybindings file: " << keybinderUser << std::endl;
        }
    }
    else
        std::cout << "Loading keybindings file: " << keybinderUser << std::endl;

    const std::string userdefault = mCfgMgr.getUserConfigPath().string() + "/gamecontrollerdb.txt";
    const std::string localdefault = mCfgMgr.getLocalPath().string() + "/gamecontrollerdb.txt";
    const std::string globaldefault = mCfgMgr.getGlobalPath().string() + "/gamecontrollerdb.txt";

    std::string userGameControllerdb;
    if (boost::filesystem::exists(userdefault)){
        userGameControllerdb = userdefault;
    }
    else
        userGameControllerdb = "";

    std::string gameControllerdb;
    if (boost::filesystem::exists(localdefault))
        gameControllerdb = localdefault;
    else if (boost::filesystem::exists(globaldefault))
        gameControllerdb = globaldefault;

    vsg::ref_ptr<vsg::Context> context = vsg::Context::create(mRenderEngine.getOrCreateDevice());
    //vsgopenmw-test-me(manual-descriptor-poolsize)
    //context->descriptorPools.push_back(
    //Engine.compileResourceHints = 
    context->copyImageCmd = vsg::CopyAndReleaseImage::create(context->stagingMemoryBufferPools);

    auto compile = mRenderEngine.createCompileManager();
    mWindowManager = std::make_unique<MWGui::WindowManager>(mWindow, context.get(), compile, mResourceSystem.get(), mCfgMgr.getLogPath().string() + std::string("/"), (mResDir / "shaders" / "gui").string(), mScriptConsoleMode, mTranslationDataStorage, mEncoding, Version::getOpenmwVersionDescription(mResDir.string()));
    mEnvironment.setWindowManager(*mWindowManager);

    mInputManager = std::make_unique<MWInput::InputManager>(mWindow, keybinderUser, keybinderUserExists, userGameControllerdb, gameControllerdb, mGrab);
    mWindowListener = std::make_unique<WindowListener>(*this);
    mInputManager->setWindowListener(mWindowListener.get());
    mEnvironment.setInputManager(*mInputManager);

    mRenderManager = std::make_unique<MWRender::RenderManager>(mRenderEngine, context, mWindowManager->node(), mWindowManager->getRenderTextures(), mResourceSystem.get(), mResDir.string()/*, *mNavigator*/);
    mLuaManager->mRenderManager = mRenderManager.get();

    mSoundManager = std::make_unique<MWSound::SoundManager>(mVFS.get(), mUseSound);
    mEnvironment.setSoundManager(*mSoundManager);

    mStateManager->pushGameState(MWState::makeLoadingScreen<Boot>(*this));
}

// Initialise and enter main loop.
void Engine::go()
{
    assert (!mContentFiles.empty());

    SDL_version sdlVersion;
    SDL_GetVersion(&sdlVersion);
    std::cout << "SDL version: " << (int)sdlVersion.major << "." << (int)sdlVersion.minor << "." << (int)sdlVersion.patch << std::endl;

    Misc::Rng::init(mRandomSeed);

    Settings::Manager settings;

    mEncoder = std::make_unique<ToUTF8::Utf8Encoder>(mEncoding);

    prepareEngine (settings);
    mRenderEngine.compile();
    auto callback = [this](double dt, bool &requiresRender, bool &quit) {
        auto state = mStateManager->getGameState();
        quit = !state;
        if (quit)
            return;
        requiresRender = frame(dt, state);
    };
    mRenderEngine.loop(callback, Settings::Manager::getFloat("framerate limit", "Video"));

    std::cout << "Quitting peacefully." << std::endl;

    Settings::Manager::saveUser((mCfgMgr.getUserConfigPath() / "settings.cfg").string());
    mLuaManager->savePermanentStorage(mCfgMgr.getUserConfigPath().string());
}

void Engine::setCompileAll (bool all)
{
    mCompileAll = all;
}

void Engine::setCompileAllDialogue (bool all)
{
    mCompileAllDialogue = all;
}

void Engine::setSoundUsage(bool soundUsage)
{
    mUseSound = soundUsage;
}

void Engine::setEncoding(const ToUTF8::FromType& encoding)
{
    mEncoding = encoding;
}

void Engine::setScriptConsoleMode (bool enabled)
{
    mScriptConsoleMode = enabled;
}

void Engine::setStartupScript (const std::string& path)
{
    mStartupScript = path;
}

void Engine::setActivationDistanceOverride (int distance)
{
    mActivationDistanceOverride = distance;
}

void Engine::setWarningsMode (int mode)
{
    mWarningsMode = mode;
}

void Engine::setScriptBlacklist (const std::vector<std::string>& list)
{
    mScriptBlacklist = list;
}

void Engine::setScriptBlacklistUse (bool use)
{
    mScriptBlacklistUse = use;
}

void Engine::setSaveGameFile(const std::string &savegame)
{
    mSaveGameFile = savegame;
}

void Engine::setRandomSeed(unsigned int seed)
{
    mRandomSeed = seed;
}

}
