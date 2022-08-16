#include <components/version/version.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/fallback/fallback.hpp>
#include <components/fallback/validate.hpp>
#include <components/debug/debugging.hpp>
#include <components/misc/rng.hpp>
#include <components/platform/platform.hpp>

#include "mwgui/debugwindow.hpp"

#include "vsgengine.hpp"
#include "options.hpp"

#include <boost/program_options/variables_map.hpp>

#if defined(_WIN32)
#include <components/windows.hpp>
// makes __argc and __argv available on windows
#include <cstdlib>
#endif

#include <filesystem>

#if (defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix))
#include <unistd.h>
#endif


using namespace Fallback;

/**
 * \brief Parses application command line and calls \ref Cfg::ConfigurationManager
 * to parse configuration files.
 *
 * Results are directly written to \ref Engine class.
 *
 * \retval true - Everything goes OK
 * \retval false - Error
 */
bool parseOptions (int argc, char** argv, OMW::Engine& engine, Files::ConfigurationManager& cfgMgr)
{
    // Create a local alias for brevity
    namespace bpo = boost::program_options;
    typedef std::vector<std::string> StringsVector;

    bpo::options_description desc = OpenMW::makeOptionsDescription();
    bpo::variables_map variables;

    Files::parseArgs(argc, argv, variables, desc);
    bpo::notify(variables);

    if (variables.count ("help"))
    {
        getRawStdout() << desc << std::endl;
        return false;
    }

    if (variables.count ("version"))
    {
        cfgMgr.readConfiguration(variables, desc, true);

        Version::Version v = Version::getOpenmwVersion(variables["resources"].as<Files::MaybeQuotedPath>().string());
        getRawStdout() << v.describe() << std::endl;
        return false;
    }

    cfgMgr.readConfiguration(variables, desc);
    Settings::Manager::load(cfgMgr);

    setupLogging(cfgMgr.getLogPath().string(), "OpenMW");
    MWGui::DebugWindow::startLogRecording();

    Version::Version v = Version::getOpenmwVersion(variables["resources"].as<Files::MaybeQuotedPath>().string());
    Log(Debug::Info) << v.describe();

    engine.setGrabMouse(!variables["no-grab"].as<bool>());

    // Font encoding settings
    std::string encoding(variables["encoding"].as<std::string>());
    Log(Debug::Info) << ToUTF8::encodingUsingMessage(encoding);
    engine.setEncoding(ToUTF8::calculateEncoding(encoding));

    // directory settings
    engine.enableFSStrict(variables["fs-strict"].as<bool>());

    Files::PathContainer dataDirs(asPathContainer(variables["data"].as<Files::MaybeQuotedPathContainer>()));

    Files::PathContainer::value_type local(variables["data-local"].as<Files::MaybeQuotedPathContainer::value_type>());
    if (!local.empty())
        dataDirs.push_back(local);

    cfgMgr.filterOutNonExistingPaths(dataDirs);

    engine.setResourceDir(variables["resources"].as<Files::MaybeQuotedPath>());
    engine.setDataDirs(dataDirs);

    // fallback archives
    StringsVector archives = variables["fallback-archive"].as<StringsVector>();
    for (StringsVector::const_iterator it = archives.begin(); it != archives.end(); ++it)
    {
        engine.addArchive(*it);
    }

    StringsVector content = variables["content"].as<StringsVector>();
    if (content.empty())
    {
        Log(Debug::Error) << "No content file given (esm/esp, nor omwgame/omwaddon). Aborting...";
        return false;
    }
    std::set<std::string> contentDedupe;
    for (const auto& contentFile : content)
    {
        if (!contentDedupe.insert(contentFile).second)
        {
            Log(Debug::Error) << "Content file specified more than once: " << contentFile << ". Aborting...";
            return false;
        }
    }

    for (auto& file : content)
    {
        engine.addContentFile(file);
    }

    StringsVector groundcover = variables["groundcover"].as<StringsVector>();
    for (auto& file : groundcover)
    {
        engine.addGroundcoverFile(file);
    }

    if (variables.count("lua-scripts"))
    {
        Log(Debug::Warning) << "Lua scripts have been specified via the old lua-scripts option and will not be loaded. "
                               "Please update them to a version which uses the new omwscripts format.";
    }

    // startup-settings
    engine.setCell(variables["start"].as<std::string>());
    engine.setSkipMenu (variables["skip-menu"].as<bool>(), variables["new-game"].as<bool>());
    if (!variables["skip-menu"].as<bool>() && variables["new-game"].as<bool>())
        Log(Debug::Warning) << "Warning: new-game used without skip-menu -> ignoring it";

    // scripts
    engine.setCompileAll(variables["script-all"].as<bool>());
    engine.setCompileAllDialogue(variables["script-all-dialogue"].as<bool>());
    engine.setScriptConsoleMode (variables["script-console"].as<bool>());
    engine.setStartupScript (variables["script-run"].as<std::string>());
    engine.setWarningsMode (variables["script-warn"].as<int>());
    engine.setScriptBlacklist (variables["script-blacklist"].as<StringsVector>());
    engine.setScriptBlacklistUse (variables["script-blacklist-use"].as<bool>());
    engine.setSaveGameFile (variables["load-savegame"].as<Files::MaybeQuotedPath>().string());

    // other settings
    Fallback::Map::init(variables["fallback"].as<FallbackMap>().mMap);
    engine.setSoundUsage(!variables["no-sound"].as<bool>());
    engine.setActivationDistanceOverride (variables["activate-dist"].as<int>());
    engine.setRandomSeed(variables["random-seed"].as<unsigned int>());

    return true;
}

int runApplication(int argc, char *argv[])
{
    Platform::init();

#ifdef __APPLE__
    std::filesystem::path binary_path = std::filesystem::absolute(std::filesystem::path(argv[0]));
    std::filesystem::current_path(binary_path.parent_path());
    setenv("OSG_GL_TEXTURE_STORAGE", "OFF", 0);
#endif

    Files::ConfigurationManager cfgMgr;
    std::unique_ptr<OMW::Engine> engine = std::make_unique<OMW::Engine>(cfgMgr);

    if (parseOptions(argc, argv, *engine, cfgMgr))
    {
        engine->go();
    }

    return 0;
}

#ifdef ANDROID
extern "C" int SDL_main(int argc, char**argv)
#else
int main(int argc, char**argv)
#endif
{
    return wrapApplication(&runApplication, argc, argv, "OpenMW", false);
}

// Platform specific for Windows when there is no console built into the executable.
// Windows will call the WinMain function instead of main in this case, the normal
// main function is then called with the __argc and __argv parameters.
#if defined(_WIN32) && !defined(_CONSOLE)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    return main(__argc, __argv);
}
#endif
