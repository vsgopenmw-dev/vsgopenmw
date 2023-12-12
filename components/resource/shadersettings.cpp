#include "shadersettings.hpp"

#include <sstream>
#include <algorithm>

#include <vsg/state/ShaderModule.h>

#include <components/fallback/fallback.hpp>
#include <components/settings/settings.hpp>

namespace
{
    void add(const std::string& identifier, bool value, std::stringstream& stream)
    {
        stream << "const bool " << identifier << " = " << (value ? "true" : "false") << ";\n";
    }
    void add(const std::string& identifier, int value, std::stringstream& stream)
    {
        stream << "const int " << identifier << " = " << value << ";\n";
    }
    void add(const std::string& identifier, float value, std::stringstream& stream)
    {
        stream << "const float " << identifier << " = " << value << ";\n";
    }

    void addFallbackBool(const std::string& setting, std::stringstream& stream)
    {
        add(setting, Fallback::Map::getBool(setting), stream);
    }
    void addFallbackInt(const std::string& setting, std::stringstream& stream)
    {
        add(setting, Fallback::Map::getInt(setting), stream);
    }
    void addFallbackFloat(const std::string& setting, std::stringstream& stream)
    {
        add(setting, Fallback::Map::getFloat(setting), stream);
    }
}

namespace Resource
{
    ShaderSettings::ShaderSettings()
    {
        std::stringstream source;
        addFallbackBool("LightAttenuation_UseConstant", source);
        addFallbackFloat("LightAttenuation_ConstantValue", source);
        addFallbackBool("LightAttenuation_UseLinear", source);
        addFallbackInt("LightAttenuation_LinearMethod", source);
        addFallbackFloat("LightAttenuation_LinearValue", source);
        addFallbackFloat("LightAttenuation_LinearRadiusMult", source);
        addFallbackBool("LightAttenuation_UseQuadratic", source);
        addFallbackInt("LightAttenuation_QuadraticMethod", source);
        addFallbackFloat("LightAttenuation_QuadraticValue", source);
        addFallbackFloat("LightAttenuation_QuadraticRadiusMult", source);
        //      fallback=LightAttenuation_OutQuadInLin,0

        objects["light.settings"] = vsg::ShaderModule::create(source.str());

        source.str("");
        add("cEnableShadows", Settings::Manager::getBool("enable shadows", "Shadows"), source);
        add("cPCSS", Settings::Manager::getBool("percentage closer soft shadows", "Shadows"), source);
        add("cShadowMapCount", Settings::Manager::getInt("number of shadow maps", "Shadows"), source);
        add("cBlockerSamples", std::clamp(Settings::Manager::getInt("blocker samples", "Shadows"), 0, 16), source);
        add("cShadowSamples", std::clamp(Settings::Manager::getInt("pcf samples", "Shadows"), 0, 16), source);
        add("cShadowMaxDist", Settings::Manager::getFloat("maximum shadow map distance", "Shadows"), source);
        add("cShadowFadeStart", Settings::Manager::getFloat("shadow fade start", "Shadows"), source);
        if (Settings::Manager::getBool("enable debug hud", "Shadows"))
            source << "#define DEBUG_SHADOW 1\n";
        objects["shadow.settings"] = vsg::ShaderModule::create(source.str());

        source.str("");
        add("cClampLighting", Settings::Manager::getBool("clamp lighting", "Shaders"), source);
        objects["material.settings"] = vsg::ShaderModule::create(source.str());

        source.str("");
        if (Settings::Manager::getBool("debug chunks", "Terrain"))
            source << "#define DEBUG_CHUNKS 1\n";
        objects["terrain.settings"] = vsg::ShaderModule::create(source.str());
    }
}
