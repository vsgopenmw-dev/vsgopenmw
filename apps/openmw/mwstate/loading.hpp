#ifndef VSGOPENMW_MWSTATE_LOADING_H
#define VSGOPENMW_MWSTATE_LOADING_H

#include <mutex>
#include <atomic>

#include <vsg/core/ref_ptr.h>

#include "gamestate.hpp"

namespace vsg
{
    class OperationThreads;
}
namespace vsgUtil
{
    class Operation;
}
namespace MWState
{
    /*
     * Progresses.
     */
    class Loading : public GameState
    {
    public:
        Loading();
        ~Loading();

        /*
         * Provides thread safe progress information.
         */
        void setComplete(float complete);
        float progressStep = 0;
        void advance();
        void setDescription(const std::string& title);
        float getComplete() const;
        std::string getDescription() const;

        /*
         * May support threading.
         */
        vsg::ref_ptr<vsg::OperationThreads> threads;

        /*
         * May respond to abort request.
         */
        std::atomic_bool abort = false;

    protected:
        mutable std::mutex mMutex;
        std::string mDescription;
        float mComplete = 0.f;
    };

    /*
     * Automatically runs threadLoad in provided OperationThreads.
     */
    class ThreadLoading : public Loading
    {
    public:
        ThreadLoading();
        ~ThreadLoading();
        bool run(float dt) override;
        virtual void threadLoad() = 0;
    private:
        vsg::ref_ptr<vsgUtil::Operation> mOperation;
    };
}
#endif
