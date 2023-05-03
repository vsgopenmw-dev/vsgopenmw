
#include "conversion.hpp"

#include <components/misc/strings/conversion.hpp>

std::string Files::pathToUnicodeString(const std::filesystem::path& path)
{
    return path.string();
}

std::string Files::pathToUnicodeString(std::filesystem::path&& path)
{
    return path.string();
}

std::filesystem::path Files::pathFromUnicodeString(std::string_view path)
{
    return { path };
}

std::filesystem::path Files::pathFromUnicodeString(std::string&& path)
{
    return { std::move(path) };
}

std::filesystem::path Files::pathFromUnicodeString(const char* path)
{
    return { path };
}
