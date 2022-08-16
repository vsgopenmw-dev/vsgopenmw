#include "imageio.hpp"

#include <iostream>
#include <sstream>

#include <vsgXchange/images.h>
#include <vsg/io/read.h>
#include <vsg/io/write.h>

#include <components/files/memorystream.hpp>
#include <components/vsgutil/share.hpp>

namespace
{
    const std::string_view pngHeader = "<89>PNG";
    vsg::ref_ptr<vsg::Options> pngOptions()
    {
        auto options = vsg::Options::create();
        options->extensionHint = ".png";
        options->readerWriters = {vsgXchange::stbi::create()};
        return options;
    }
    vsg::ref_ptr<vsg::Options> tgaOptions()
    {
        auto options = vsg::Options::create();
        options->extensionHint = ".tga";
        options->readerWriters = {vsgXchange::stbi::create()};
        return options;
    }
}
namespace MWRender
{
    vsg::ref_ptr<vsg::Data> readImageFromMemory(const std::vector<char> &encoded)
    {
        vsg::ref_ptr<vsg::Options> options;
        if (encoded.size() >= pngHeader.size() && std::string_view(&encoded[0], pngHeader.size()) == pngHeader)
            options = vsgUtil::share<vsg::Options>(pngOptions);
        else
            options = vsgUtil::share<vsg::Options>(tgaOptions);

        Files::IMemStream in(encoded.data(), encoded.size());
        vsgXchange::stbi stbi;
        auto obj = stbi.read(in, options);
        if (obj)
        {
            if (auto data = obj->cast<vsg::Data>())
                return vsg::ref_ptr<vsg::Data>(data);
        }
        std::cerr << "!data(readImageFromMemory, size=" << encoded.size()<< ")" << std::endl;
        return {};
    }

    std::vector<char> writeImageToMemory(vsg::ref_ptr<vsg::Data> data)
    {
        std::ostringstream out;
        vsgXchange::stbi stbi;
        auto result = stbi.write(data, out, vsgUtil::share<vsg::Options>(pngOptions));
        if (!result)
        {
            std::cerr << "!result(writeImageToMemory)" << std::endl;
            return {};
        }
        else
        {
            std::string dataStr = out.str();
            return std::vector<char>(dataStr.begin(), dataStr.end());
        }
    }
}

