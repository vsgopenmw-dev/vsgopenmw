#ifndef COMPONENTS_SETTINGS_H
#define COMPONENTS_SETTINGS_H

#include "categories.hpp"

#include <string>
#include <string_view>
#include <vector>

#include <filesystem>
#include <osg/Vec2f>
#include <osg/Vec3f>

namespace Files
{
    // vsgopenmw-fixme(dependency-policy)
    struct ConfigurationManager;
}

namespace Settings
{
    // vsgopenmw-fixme(find-my-place)
    enum class WindowMode
    {
        Fullscreen = 0,
        WindowedFullscreen,
        Windowed
    };

    // vsgopenmw-fixme(global-state)
    ///
    /// \brief Settings management (can change during runtime)
    ///
    class Manager
    {
    public:
        static CategorySettingValueMap mDefaultSettings;
        static CategorySettingValueMap mUserSettings;

        static CategorySettingVector mChangedSettings;
        ///< tracks all the settings that were changed since the last apply() call

        static void clear();
        ///< clears all settings and default settings

        static std::filesystem::path load(const Files::ConfigurationManager& cfgMgr, bool loadEditorSettings = false);
        ///< load settings from all active config dirs. Returns the path of the last loaded file.

        static void saveUser(const std::filesystem::path& file);
        ///< save user settings to file

        static void resetPendingChanges();
        ///< resets the list of all pending changes

        static void resetPendingChanges(const CategorySettingVector& filter);
        ///< resets only the pending changes listed in the filter

        static CategorySettingVector getPendingChanges();
        ///< returns the list of changed settings

        static CategorySettingVector getPendingChanges(const CategorySettingVector& filter);
        ///< returns the list of changed settings intersecting with the filter

        static int getInt(std::string_view setting, std::string_view category);
        static std::uint64_t getUInt64(std::string_view setting, std::string_view category);
        static std::size_t getSize(std::string_view setting, std::string_view category);
        static float getFloat(std::string_view setting, std::string_view category);
        static double getDouble(std::string_view setting, std::string_view category);
        static const std::string& getString(std::string_view setting, std::string_view category);
        static std::vector<std::string> getStringArray(std::string_view setting, std::string_view category);
        static bool getBool(std::string_view setting, std::string_view category);
        static osg::Vec2f getVector2(std::string_view setting, std::string_view category);
        static osg::Vec3f getVector3(std::string_view setting, std::string_view category);

        static void setInt(std::string_view setting, std::string_view category, int value);
        static void setUInt64(std::string_view setting, std::string_view category, std::uint64_t value);
        static void setFloat(std::string_view setting, std::string_view category, float value);
        static void setDouble(std::string_view setting, std::string_view category, double value);
        static void setString(std::string_view setting, std::string_view category, const std::string& value);
        static void setStringArray(
            std::string_view setting, std::string_view category, const std::vector<std::string>& value);
        static void setBool(std::string_view setting, std::string_view category, bool value);
        static void setVector2(std::string_view setting, std::string_view category, osg::Vec2f value);
        static void setVector3(std::string_view setting, std::string_view category, osg::Vec3f value);
    };

}

#endif // COMPONENTS_SETTINGS_H
