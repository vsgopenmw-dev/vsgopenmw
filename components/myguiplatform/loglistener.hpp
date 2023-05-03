#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_LOGLISTENER_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_LOGLISTENER_H

#include <filesystem>
#include <fstream>
#include <string>

#include <MyGUI_ConsoleLogListener.h>
#include <MyGUI_ILogListener.h>
#include <MyGUI_LevelLogFilter.h>
#include <MyGUI_LogSource.h>

namespace MyGUIPlatform
{

    /// \brief  Custom MyGUI::ILogListener interface implementation
    /// being able to portably handle UTF-8 encoded path.
    /// \todo try patching MyGUI to make this easier
    class CustomLogListener : public MyGUI::ILogListener
    {
    public:
        CustomLogListener(const std::filesystem::path& name)
            : mFileName(name)
        {
        }

        ~CustomLogListener() {}

        void open() override;
        void close() override;
        void flush() override;

        void log(const std::string& _section, MyGUI::LogLevel _level, const struct tm* _time,
            const std::string& _message, const char* _file, int _line) override;

    private:
        std::ofstream mStream;
        std::filesystem::path mFileName;
    };

    /// \brief Helper class holding data that required during
    /// MyGUI log creation
    class LogFacility
    {
        MyGUI::ConsoleLogListener mConsole;
        CustomLogListener mFile;
        MyGUI::LevelLogFilter mFilter;
        MyGUI::LogSource mSource;

    public:
        LogFacility(const std::filesystem::path& output, bool console)
            : mFile(output)
        {
            mConsole.setEnabled(console);
            mFilter.setLoggingLevel(MyGUI::LogLevel::Info);

            mSource.addLogListener(&mFile);
            mSource.addLogListener(&mConsole);
            mSource.setLogFilter(&mFilter);

            mSource.open();
        }

        MyGUI::LogSource* getSource() { return &mSource; }
    };

}

#endif
