#include "hash.hpp"
#include "conversion.hpp"

// #include <extern/smhasher/MurmurHash3.h>

#include <array>
#include <cstdint>
#include <istream>
#include <string>

namespace Files
{
    std::array<std::uint64_t, 2> getHash(const std::filesystem::path& fileName, std::istream& stream)
    {
        std::array<std::uint64_t, 2> hash{ 0, 0 };
        /*
        try
        {
            const auto start = stream.tellg();
            const auto exceptions = stream.exceptions();
            stream.exceptions(std::ios_base::badbit);
            while (stream)
            {
                std::array<char, 4096> value;
                stream.read(value.data(), value.size());
                const std::streamsize read = stream.gcount();
                if (read == 0)
                    break;
                std::array<std::uint64_t, 2> blockHash {0, 0};
                MurmurHash3_x64_128(value.data(), static_cast<int>(read), hash.data(), blockHash.data());
                hash = blockHash;
            }
            stream.clear();
            stream.exceptions(exceptions);
            stream.seekg(start);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Error while reading \"" + Files::pathToUnicodeString(fileName) + "\" to get hash:
        " + e.what());
        }
        */
        return hash;
    }
}
