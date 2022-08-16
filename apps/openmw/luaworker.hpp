#ifndef VSGOPENMW_LUAWORKER_H
#define VSGOPENMW_LUAWORKER_H

#include <thread>
#include <condition_variable>
#include <mutex>

#include "mwlua/luamanagerimp.hpp"

#include <components/settings/settings.hpp>

namespace OMW
{
    class LuaWorker
    {
    public:
        explicit LuaWorker(MWLua::LuaManager* luaMgr) : mLuaManager(luaMgr)
        {
            if (Settings::Manager::getInt("lua num threads", "Lua") > 0)
                mThread = std::thread([this]{ threadBody(); });
        };
        ~LuaWorker()
        {
            join();
        }
        void allowUpdate()
        {
            if (!mThread)
                return;
            {
                std::lock_guard<std::mutex> lk(mMutex);
                mUpdateRequest = true;
            }
            mCV.notify_one();
        }
        void finishUpdate()
        {
            if (mThread)
            {
                std::unique_lock<std::mutex> lk(mMutex);
                mCV.wait(lk, [&]{ return !mUpdateRequest; });
            }
            else
                update();
        };
        void join()
        {
            if (mThread)
            {
                {
                    std::lock_guard<std::mutex> lk(mMutex);
                    mJoinRequest = true;
                }
                mCV.notify_one();
                mThread->join();
            }
        }
    private:
        void update()
        {
            mLuaManager->update();
        }
        void threadBody()
        {
            while (true)
            {
                std::unique_lock<std::mutex> lk(mMutex);
                mCV.wait(lk, [&]{ return mUpdateRequest || mJoinRequest; });
                if (mJoinRequest)
                    break;
                update();
                mUpdateRequest = false;
                lk.unlock();
                mCV.notify_one();
            }
        }
        MWLua::LuaManager* mLuaManager;
        std::mutex mMutex;
        std::condition_variable mCV;
        bool mUpdateRequest = false;
        bool mJoinRequest = false;
        std::optional<std::thread> mThread;
    };
}

#endif
