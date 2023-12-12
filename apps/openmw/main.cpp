#include <components/debug/debuglog.hpp>
#include <components/fallback/fallback.hpp>
#include <components/fallback/validate.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/misc/rng.hpp>
#include <components/version/version.hpp>
#include <components/settings/settings.hpp>

#include "options.hpp"
#include "vsgengine.hpp"

#include <boost/program_options/variables_map.hpp>

#include <filesystem>

/**
 * \brief Parses application command line and calls \ref Cfg::ConfigurationManager
 * to parse configuration files.
 */
std::optional<OMW::Arguments> parseArgs(int argc, char** argv, Files::ConfigurationManager& cfgMgr)
{
    // Create a local alias for brevity
    namespace bpo = boost::program_options;
    typedef std::vector<std::string> StringsVector;

    bpo::options_description desc = OpenMW::makeOptionsDescription();
    bpo::variables_map variables;

    Files::parseArgs(argc, argv, variables, desc);
    bpo::notify(variables);

    if (variables.count("help"))
    {
        std::cout << desc << std::endl;
        return {};
    }

    if (variables.count("version"))
    {
        std::cout << Version::getOpenmwVersionDescription() << std::endl;
        //getRawStdout() << Version::getOpenmwVersionDescription() << std::endl;
        return {};
    }

    cfgMgr.readConfiguration(variables, desc);

    Log(Debug::Info) << Version::getOpenmwVersionDescription();

    Settings::Manager::load(cfgMgr);

    /*
    MWGui::DebugWindow::startLogRecording();
*/
    OMW::Arguments args;
    std::string encoding(variables["encoding"].as<std::string>());
    Log(Debug::Info) << ToUTF8::encodingUsingMessage(encoding);
    args.encoding = ToUTF8::calculateEncoding(encoding);

    Files::PathContainer dataDirs(asPathContainer(variables["data"].as<Files::MaybeQuotedPathContainer>()));

    Files::PathContainer::value_type local(variables["data-local"].as<Files::MaybeQuotedPathContainer::value_type>());
    if (!local.empty())
        dataDirs.push_back(local);

    args.resourceDir = variables["resources"].as<Files::MaybeQuotedPath>();
    args.dataDirs = dataDirs;

    args.archives = variables["fallback-archive"].as<StringsVector>();

    StringsVector content = variables["content"].as<StringsVector>();
    if (content.empty())
    {
        Log(Debug::Error) << "No content file given (esm/esp, nor omwgame/omwaddon). Aborting...";
        return {};
    }
    std::set<std::string> contentDedupe;
    for (const auto& contentFile : content)
    {
        if (!contentDedupe.insert(contentFile).second)
        {
            Log(Debug::Error) << "Content file specified more than once: " << contentFile << ". Aborting...";
            return {};
        }
    }

    args.contentFiles = content;

    // startup-settings
    args.saveGameFile = variables["load-savegame"].as<Files::MaybeQuotedPath>();
    args.cellName = variables["start"].as<std::string>();
    args.skipMenu = variables["skip-menu"].as<bool>(), variables["new-game"].as<bool>();

    // scripts
    args.compileAll = variables["script-all"].as<bool>();
    args.compileAllDialogue = variables["script-all-dialogue"].as<bool>();
    args.scriptConsoleMode = variables["script-console"].as<bool>();
    args.startupScript = variables["script-run"].as<std::string>();
    args.warningsMode = variables["script-warn"].as<int>();
    auto& scriptBlacklistString = variables["script-blacklist"].as<StringsVector>();
    if (variables["script-blacklist-use"].as<bool>())
        for (const auto& blacklistString : scriptBlacklistString)
        {
            args.scriptBlacklist.push_back(ESM::RefId::stringRefId(blacklistString));
        }

    // other settings
    Fallback::Map::init(variables["fallback"].as<Fallback::FallbackMap>().mMap);
    args.useSound = !variables["no-sound"].as<bool>();
    args.activationDistanceOverride = variables["activate-dist"].as<int>();
    args.randomSeed = variables["random-seed"].as<unsigned int>();
    args.grabMouse = !variables["no-grab"].as<bool>();
    return args;
}

int main(int argc, char** argv)
{
    Files::ConfigurationManager cfgMgr;
    OMW::Engine engine(cfgMgr);

    if (auto args = parseArgs(argc, argv, cfgMgr))
        engine.go(*args);

    return 0;
}
