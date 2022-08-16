#include "vfs.hpp"

#include <memory>
#include <string>
#include <filesystem>
#include <fstream>

#include <MyGUI_DataFileStream.h>

#include <components/vfs/manager.hpp>

namespace MyGUIPlatform
{

MyGUI::IDataStream *vfs::getData(const std::string &name) const
{
    Files::IStreamPtr streamPtr;
    if (name.find("/") == std::string::npos)
    {
        for (auto &f : mFolders)
        {
            auto path = f + "/" + name;
            if (!mVfs->exists(path)) continue;
            streamPtr = mVfs->/*search*/get(path);
        }
        if (!streamPtr)
            throw std::runtime_error("!vfs::getData(" + name + ")");
    }
    else
        streamPtr = mVfs->get(name);
    return new MyGUI::DataStream(streamPtr.release());
}

void vfs::freeData(MyGUI::IDataStream *data)
{
    delete data;
}

bool vfs::isDataExist(const std::string &name) const
{
    return !getDataPath(name).empty();
}

const MyGUI::VectorString &vfs::getDataListNames(const std::string &pattern) const
{
    throw std::runtime_error("unimplemented(getDataListNames)");
}

const std::string &vfs::getDataPath(const std::string &name) const
{
    static std::string result;
    result.clear();
/*
    if (name.empty())
        return mPrefix;
        */
    if (mVfs->exists(name))
        return name;
    for (auto &f : mFolders)
    {
        result = f + "/" + name;
        if (mVfs->exists(result))
            return result;
    }
    return result;
}

}
