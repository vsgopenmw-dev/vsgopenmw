#include "shadersettings.hpp"

#include <sstream>

#include <vsg/state/ShaderModule.h>

#include <components/fallback/fallback.hpp>

namespace
{
    void addBool(const std::string &setting, std::stringstream &stream)
    {
        stream << "const bool " << setting << " = " << (Fallback::Map::getBool(setting) ? "true" : "false") << ";" << std::endl;
    }
    void addInt(const std::string &setting, std::stringstream &stream)
    {
        stream << "const int " << setting << " = " << Fallback::Map::getInt(setting) << ";" << std::endl;
    }
    void addFloat(const std::string &setting, std::stringstream &stream)
    {
        stream << "const float " << setting << " = " << Fallback::Map::getFloat(setting) << ";"<< std::endl;
    }
}

namespace Resource
{
    ShaderSettings::ShaderSettings()
    {
        std::stringstream source;
        addBool("LightAttenuation_UseConstant", source);
        addInt("LightAttenuation_ConstantValue", source);
        addBool("LightAttenuation_UseLinear", source);
        addInt("LightAttenuation_LinearMethod", source);
        addFloat("LightAttenuation_LinearValue", source);
        addFloat("LightAttenuation_LinearRadiusMult", source);
        addBool("LightAttenuation_UseQuadratic", source);
        addInt("LightAttenuation_QuadraticMethod", source);
        addFloat("LightAttenuation_QuadraticValue", source);
        addFloat("LightAttenuation_QuadraticRadiusMult", source);
//      fallback=LightAttenuation_OutQuadInLin,0

        this->object = vsg::ShaderModule::create(source.str());
        this->filename = "settings";
    }
}
