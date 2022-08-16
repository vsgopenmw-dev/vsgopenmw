#ifndef VSGOPENMW_MYGUIPLATFORM_VFS_H
#define VSGOPENMW_MYGUIPLATFORM_VFS_H

#include <string>

#include <MyGUI_DataManager.h>

namespace VFS
{
    class Manager;
}
namespace MyGUIPlatform
{

class vfs : public MyGUI::DataManager
{
public:
    void initialise() {}
    void shutdown() {}

    vfs(const VFS::Manager *in_vfs, const std::vector<std::string> &folders) : mVfs(in_vfs), mFolders(folders) {}

    /** Get data stream from specified resource name.
        @param _name Resource name (usually file name).
    */
    MyGUI::IDataStream* getData(const std::string& _name) const override;

    /** Free data stream.
        @param _data Data stream.
    */
    void freeData(MyGUI::IDataStream* _data) override;

    /** Is data with specified name exist.
        @param _name Resource name.
    */
    bool isDataExist(const std::string& _name) const override;

    /** Get all data names with names that matches pattern.
        @param _pattern Pattern to match (for example "*.layout").
    */
    const MyGUI::VectorString& getDataListNames(const std::string& _pattern) const override;

    /** Get full path to data.
        @param _name Resource name.
        @return Return full path to specified data.
    */
    const std::string& getDataPath(const std::string& _name) const override;

private:
    const VFS::Manager* mVfs;
    std::vector<std::string> mFolders;
};

}

#endif
