#include "loading.hpp"

#include <vsg/threading/OperationThreads.h>

#include <components/vsgutil/operation.hpp>

namespace MWState
{
    namespace
    {
        struct Operation : public vsgUtil::Operation
        {
            ThreadLoading* loading{};
            Operation(ThreadLoading* in_loading)
                : loading(in_loading)
            {
            }
            void operate() override
            {
                loading->threadLoad();
            }
        };
    }

    Loading::Loading() {}

    Loading::~Loading() {}

    float Loading::getComplete() const
    {
        std::lock_guard<std::mutex> lk(mMutex);
        return mComplete;
    }

    void Loading::setComplete(float complete)
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mComplete = complete;
    }

    void Loading::advance()
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mComplete += progressStep;
    }

    std::string Loading::getDescription() const
    {
        std::lock_guard<std::mutex> lk(mMutex);
        return mDescription;
    }

    void Loading::setDescription(const std::string& desc)
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mDescription = desc;
    }

    ThreadLoading::ThreadLoading() {}

    ThreadLoading::~ThreadLoading()
    {
        if (mOperation)
            mOperation->wait();
    }

    bool ThreadLoading::run(float dt)
    {
        if (!threads)
            return false;
        if (!mOperation)
        {
            mOperation = new Operation(this);
            threads->add(mOperation);
            return true;
        }
        bool done = mOperation->done;
        if (done)
            mOperation->ensure();
        return !done;
    }
}
