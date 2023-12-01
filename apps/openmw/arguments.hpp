#ifndef VSGOPENMW_ARGUMENTS_H
#define VSGOPENMW_ARGUMENTS_H

#include <components/files/multidircollection.hpp>
#include <components/to_utf8/to_utf8.hpp>
#include <components/esm/refid.hpp>

namespace OMW
{
    /*
     * Arguments is simply a struct of options that can vary per invocation/run of the game/application, typically specified on the command line.
     */
    struct Arguments
    {
        std::vector<std::string> contentFiles;
        Files::PathContainer dataDirs;
        std::vector<std::string> archives;
        std::filesystem::path resourceDir;
        ToUTF8::FromType encoding = ToUTF8::WINDOWS_1252;

        bool useSound = true;
        bool grabMouse = true;

        int activationDistanceOverride = -1;
        unsigned int randomSeed = 0;

        std::vector<ESM::RefId> scriptBlacklist;
        bool compileAll = false;
        bool compileAllDialogue = false;
        int warningsMode = 1;
        bool scriptConsoleMode = false;

        std::filesystem::path saveGameFile;
        std::string cellName;
        std::string startupScript;
        bool newGame = false;
        bool skipMenu = false;
    };
}

#endif
