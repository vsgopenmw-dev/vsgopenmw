#ifndef OPENMW_MWLUA_WORKER_H
#define OPENMW_MWLUA_WORKER_H

#include <vector>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

namespace MWLua
{
    class LuaManager;

    class Worker
    {
    public:
        explicit Worker(LuaManager& manager);

        ~Worker();

        void allowUpdate(float dt);

        void finishUpdate(float dt);

        void join();

    private:
        void update(float dt);

        void run() noexcept;

        LuaManager& mManager;
        std::mutex mMutex;
        std::condition_variable mCV;
        std::vector<float /*dt*/> mUpdateRequest;
        bool mJoinRequest = false;
        std::optional<std::thread> mThread;
    };
}

#endif // OPENMW_MWLUA_LUAWORKER_H
