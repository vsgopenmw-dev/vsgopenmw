#ifndef VSGOPENMW_MWSTATE_LOADING_H
#define VSGOPENMW_MWSTATE_LOADING_H

#include <thread>
#include <mutex>

#include "gamestate.hpp"

namespace MWState
{
    /*
     * Progresses.
     */
    class Loading : public GameState
    {
    protected:
        std::optional<std::thread> mThread;
        mutable std::mutex mMutex;
        std::string mError;
        std::string mDescription;
        float mComplete = 0.f;
        bool mThreadDone = false;
        void threadEntryPoint();
    public:
        ~Loading();
        void setComplete(float complete);
        float progressStep = 0;
        void advance();
        void setDescription(const std::string &title);
        bool run(float dt) override;

        float getComplete() const;
        std::string getDescription() const;
        std::atomic_bool abort{};

        virtual void threadLoad() {}
    };
}
#endif
