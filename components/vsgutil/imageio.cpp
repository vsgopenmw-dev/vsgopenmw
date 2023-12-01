#include "imageio.hpp"

#include <iostream>
#include <sstream>

#include <vsg/io/mem_stream.h>
#include <vsg/io/read.h>
#include <vsg/io/write.h>
#include <vsgXchange/images.h>

namespace
{
    const std::string_view pngHeader = "<89>PNG";
    const std::string_view jpgHeader = "ÿØÿà";
    vsg::ref_ptr<vsg::Options> rwOptions(const char* ext)
    {
        auto options = vsg::Options::create();
        options->extensionHint = ext;
        options->readerWriters = { vsgXchange::stbi::create() };
        return options;
    }
}
namespace vsgUtil
{
    vsg::ref_ptr<vsg::Data> readImageFromMemory(const std::vector<char>& encoded)
    {
        vsg::ref_ptr<vsg::Options> options;
        if (encoded.size() >= pngHeader.size() && std::string_view(&encoded[0], pngHeader.size()) == pngHeader)
            options = rwOptions(".png");
        else if (encoded.size() >= jpgHeader.size() && std::string_view(&encoded[0], jpgHeader.size()) == jpgHeader)
            options = rwOptions(".jpg");
        else
            options = rwOptions(".tga");

        vsg::mem_stream in(reinterpret_cast<const uint8_t*>(encoded.data()), encoded.size());
        vsgXchange::stbi stbi;
        auto obj = stbi.read(in, options);
        if (obj)
        {
            if (auto data = obj->cast<vsg::Data>())
                return vsg::ref_ptr<vsg::Data>(data);
        }
        std::cerr << "!readImageFromMemory(size=" << encoded.size() << ")" << std::endl;
        return {};
    }

    std::vector<char> writeImageToMemory(vsg::ref_ptr<vsg::Data> data, const char* ext)
    {
        std::ostringstream out;
        vsgXchange::stbi stbi;
        auto result = stbi.write(data, out, rwOptions(ext));
        if (!result)
        {
            std::cerr << "!writeImageToMemory(size=" << data->dataSize() << ")" << std::endl;
            return {};
        }
        else
        {
            std::string dataStr = out.str();
            return std::vector<char>(dataStr.begin(), dataStr.end());
        }
    }
}
