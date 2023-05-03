#ifndef CONTENTLOADER_HPP
#define CONTENTLOADER_HPP

#include <filesystem>

namespace MWState
{
    class Loading;
}
namespace MWWorld
{

    struct ContentLoader
    {
        virtual ~ContentLoader() = default;

        virtual void load(const std::filesystem::path& filepath, int& index, MWState::Loading& state) = 0;
    };

} /* namespace MWWorld */

#endif /* CONTENTLOADER_HPP */
